#!/usr/bin/env python3
"""Level-match the bank on body RMS, letting peak float.

Matching on PEAK would be wrong here: on an 808 the peak IS the knock, so
normalising it flattens exactly the variation that makes one preset hit harder
than another. Matching on RMS holds perceived loudness steady while leaving each
preset's transient free to stand as far above its own body as it was designed to.

A struck envelope (ampCurve below 1) carries less sustained energy than the
plateau it replaced, so the reformed bank came out about 2.3 dB quieter. This
gives that back per preset rather than with one global trim, which also tightens
the spread - a factory bank where presets jump in level between auditions reads
as unfinished however good each one is.

  usage: level_match.py <probe.json> [target_rms_db]
"""
import importlib.util
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
GENERATOR = ROOT / "tools" / "generate_factory_presets.py"

PEAK_CEILING = -1.2   # leave headroom under the plugin's own -0.8 dBFS ceiling
OUTPUT_RANGE = (-24.0, 12.0)


def main():
    probe = json.load(open(sys.argv[1]))
    target = float(sys.argv[2]) if len(sys.argv) > 2 else -15.0

    spec = importlib.util.spec_from_file_location("_gen", GENERATOR)
    gen = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(gen)

    measured = {p["name"]: p for p in probe}
    out, moved = [], []

    for category, recipes in gen.RECIPES.items():
        base = dict(gen.DEFAULT_PARAMETERS)
        base.update(gen.CATEGORY_BASES.get(category, {}))
        presets = []
        for recipe in recipes:
            overrides = dict(recipe["overrides"])
            row = measured.get(recipe["name"])
            if row is not None:
                current = float(overrides.get("output", base.get("output", -6)))
                delta = target - float(row["rmsDb"])
                # Never let the trim push this preset's peak into the ceiling.
                headroom = PEAK_CEILING - float(row["peakDb"])
                delta = min(delta, headroom)
                new_output = max(OUTPUT_RANGE[0], min(OUTPUT_RANGE[1], current + delta))
                overrides["output"] = round(new_output, 2)
                moved.append(abs(new_output - current))
            presets.append({
                "name": recipe["name"],
                "description": recipe["description"],
                "tags": recipe["tags"],
                "identity": "level-matched",
                "separatedFrom": "unchanged",
                "overrides": [{"param": k, "value": (1.0 if v is True else 0.0 if v is False else float(v))}
                              for k, v in overrides.items()],
            })
        out.append({"proposal": {"category": category, "territory": "level",
                                 "approach": "rms match", "presets": presets}})

    rms = sorted(float(p["rmsDb"]) for p in probe)
    print(f"before: rms median {rms[len(rms)//2]:.2f} dB, spread p10-p90 "
          f"{rms[len(rms)//10]:.2f}..{rms[(9*len(rms))//10]:.2f}")
    print(f"target {target:.1f} dB; median trim {sorted(moved)[len(moved)//2]:.2f} dB, "
          f"max {max(moved):.2f} dB")

    target_path = ROOT / "build" / "macos" / "leveled.json"
    target_path.write_text(json.dumps({"categories": out}, indent=1))
    print(f"wrote {target_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
