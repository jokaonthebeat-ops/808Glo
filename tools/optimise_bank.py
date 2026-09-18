#!/usr/bin/env python3
"""Move colliding presets apart using rendered audio as the objective.

Hand-set ranges got the bank from 77 near-clones to 49, but every further
hand-tweak traded one goal for the other: tightening the clipper for punch cost
distinctness, reserving decay bands for separation cost within-category spread.
This searches instead of guesses - for each preset that still has a near-clone it
renders a batch of candidate variants and keeps whichever one actually measures
furthest from the rest of the bank.

Three guards keep this from optimising into incoherence, which is the failure
mode a sibling project hit when a distinctness metric scored its worst-sounding
bank highest:

  1. Candidates never leave the category's musically vetted ranges, so a
     Clean & Sub preset cannot be handed a Distorted preset's drive.
  2. A candidate is rejected if it is quieter in the knock than the preset it
     replaces by more than a small tolerance - distinctness may not be bought
     with punch, which is the thing that was asked for.
  3. Only presets that are ACTUALLY colliding are moved. Presets that already
     stand alone are left exactly as they are.

  usage: optimise_bank.py <rounds> [candidates-per-preset]
"""
import importlib.util
import json
import math
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))
from analyze_probe import FEATURES, LOG_FEATURES  # noqa: E402

EVAL = ROOT / "build" / "macos" / "808GloPresetEval"
WORK = ROOT / "build" / "macos"
CLONE_THRESHOLD = 0.35
PUNCH_TOLERANCE = 0.35   # dB of knock a move may cost

# Axes the search is allowed to move, in rough order of measured authority.
MOVABLE = ["decay", "ampCurve", "drive", "body", "harmonics", "compressor",
           "clipper", "sustain", "pitchDrop", "pitchDecay", "attack", "click",
           "punch", "harmonicBalance", "tone", "clickTone"]

# Decay is the highest-authority axis, so a search told only to maximise
# distance simply makes everything long: left free it pushed Trap from 1215 ms
# to 2135 ms and Distorted from 900 to 1890. Measurably more distinct, musically
# wrong - nobody reaches for a two-second trap 808. These bands are the musical
# constraint the metric does not contain, and they are deliberately allowed to
# overlap between categories, because character (not length) is what separates
# categories from each other.
MUSICAL_DECAY = {
    "Short Punch": (110, 560),   "Detroit": (260, 820),
    "Trap": (620, 1650),         "Mix Ready": (600, 1500),
    "Distorted": (520, 1900),    "Drill": (900, 2300),
    "Clean & Sub": (1150, 3100), "West Coast": (1200, 2900),
    "Long Glide": (2200, 4800),  "Experimental": (200, 4200),
}


def load_generator():
    spec = importlib.util.spec_from_file_location("_gen", ROOT / "tools" / "generate_factory_presets.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def normalise(rows):
    stats = {}
    for feature in FEATURES:
        values = []
        for row in rows:
            v = float(row[feature])
            if feature in LOG_FEATURES:
                v = math.log10(max(v, 1e-4) + 1e-4)
            values.append(v)
        ordered = sorted(values)
        n = len(ordered)
        median = ordered[n // 2]
        q1, q3 = ordered[n // 4], ordered[(3 * n) // 4]
        stats[feature] = (median, max(q3 - q1, 1e-6))
    return stats


def vector(row, stats):
    out = {}
    for feature, (median, spread) in stats.items():
        v = float(row[feature])
        if feature in LOG_FEATURES:
            v = math.log10(max(v, 1e-4) + 1e-4)
        out[feature] = (v - median) / spread
    return out


def distance(a, b):
    total = weight_total = 0.0
    for feature, weight in FEATURES.items():
        d = a[feature] - b[feature]
        total += weight * d * d
        weight_total += weight
    return math.sqrt(total / weight_total)


def category_ranges(gen):
    """Each category's own span per axis, from the bank as it currently stands."""
    ranges = {}
    legal = {k: (float(lo), float(hi)) for k, (lo, hi) in gen.PARAMETER_RANGES.items()}
    for category, recipes in gen.RECIPES.items():
        base = dict(gen.DEFAULT_PARAMETERS)
        base.update(gen.CATEGORY_BASES.get(category, {}))
        span = {}
        for axis in MOVABLE:
            values = []
            for recipe in recipes:
                v = recipe["overrides"].get(axis, base.get(axis))
                values.append(1.0 if v is True else 0.0 if v is False else float(v))
            lo, hi = min(values), max(values)
            if hi - lo < 1e-9:
                pad = max(abs(lo) * 0.2, (legal[axis][1] - legal[axis][0]) * 0.02)
                lo, hi = lo - pad, hi + pad
            if axis == "decay":
                lo, hi = MUSICAL_DECAY[category]
            span[axis] = (max(lo, legal[axis][0]), min(hi, legal[axis][1]))
        ranges[category] = span
    return ranges


def evaluate(candidates, tag):
    infile = WORK / f"cand_{tag}.json"
    outfile = WORK / f"cand_{tag}_out.json"
    infile.write_text(json.dumps(candidates))
    subprocess.run([str(EVAL), str(infile), str(outfile)],
                   cwd=ROOT, check=True, capture_output=True)
    return {row["id"]: row for row in json.loads(outfile.read_text())}


def main():
    rounds = int(sys.argv[1]) if len(sys.argv) > 1 else 3
    per_preset = int(sys.argv[2]) if len(sys.argv) > 2 else 10

    gen = load_generator()
    ranges = category_ranges(gen)

    bank = json.loads((ROOT / "Resources" / "FactoryPresets.json").read_text())["presets"]
    params = {p["name"]: {k: (1.0 if v is True else 0.0 if v is False else float(v))
                          for k, v in p["parameters"].items()} for p in bank}
    category = {p["name"]: p["category"] for p in bank}
    names = [p["name"] for p in bank]

    features = evaluate([{"id": n, "parameters": params[n]} for n in names], "base")

    for round_index in range(rounds):
        stats = normalise([features[n] for n in names])
        vectors = {n: vector(features[n], stats) for n in names}

        nearest = {}
        for n in names:
            nearest[n] = min(distance(vectors[n], vectors[m]) for m in names if m != n)
        clones = [n for n in names if nearest[n] < CLONE_THRESHOLD]
        median_nn = sorted(nearest.values())[len(names) // 2]
        print(f"round {round_index}: {len(clones)} clones, median NN {median_nn:.3f}")
        if not clones:
            break

        # One deterministic candidate sweep per colliding preset: vary a rotating
        # subset of axes so successive rounds explore different directions.
        candidates = []
        for slot, name in enumerate(clones):
            span = ranges[category[name]]
            for k in range(per_preset):
                variant = dict(params[name])
                # Golden-ratio offsets give spread-out positions without RNG, so
                # a rerun of this script reproduces exactly.
                for a in range(3):
                    axis = MOVABLE[(slot + k * 3 + a + round_index * 5) % len(MOVABLE)]
                    lo, hi = span[axis]
                    position = ((k + 1) * 0.6180339887 + a * 0.2360679 + round_index * 0.1) % 1.0
                    variant[axis] = lo + (hi - lo) * position
                candidates.append({"id": f"{name}@@{k}", "parameters": variant,
                                   "_name": name, "_params": variant})

        print(f"  evaluating {len(candidates)} candidates...")
        measured = evaluate([{"id": c["id"], "parameters": c["parameters"]} for c in candidates],
                            f"r{round_index}")

        accepted = 0
        for name in clones:
            best, best_score = None, nearest[name]
            for candidate in candidates:
                if candidate["_name"] != name:
                    continue
                row = measured.get(candidate["id"])
                if row is None:
                    continue
                # Distinctness may not be bought with knock.
                if row["punchDb"] < features[name]["punchDb"] - PUNCH_TOLERANCE:
                    continue
                probe_vector = vector(row, stats)
                score = min(distance(probe_vector, vectors[m]) for m in names if m != name)
                if score > best_score:
                    best, best_score = candidate, score
            if best is not None:
                params[name] = best["_params"]
                features[name] = measured[best["id"]]
                accepted += 1
        print(f"  moved {accepted} presets")
        if accepted == 0:
            print("  no candidate improved on its preset; stopping")
            break

    out = []
    for cat, recipes in gen.RECIPES.items():
        presets = []
        for recipe in recipes:
            name = recipe["name"]
            presets.append({
                "name": name,
                "description": recipe["description"],
                "tags": recipe["tags"],
                "identity": "measurement-placed",
                "separatedFrom": "maximised nearest-neighbour distance on rendered audio",
                "overrides": [{"param": k, "value": v} for k, v in params[name].items()],
            })
        out.append({"proposal": {"category": cat, "territory": "optimised",
                                 "approach": "search", "presets": presets}})
    target = WORK / "optimised.json"
    target.write_text(json.dumps({"categories": out}, indent=1))
    print(f"wrote {target}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
