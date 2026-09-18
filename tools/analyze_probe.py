#!/usr/bin/env python3
"""Measure how distinct the factory bank actually SOUNDS.

Distances are computed on rendered-audio features, never on parameter values.
A parameter-space metric rewards unrelated settings, so optimising it optimises
for incoherence - it scored the worst-sounding bank of a sibling project
highest. Every preset is rendered at one note, because per-preset registers
hide the collisions a player hears on a single keyboard.
"""
import json
import math
import sys
from collections import defaultdict

# Perceptual weights: what a listener actually uses to tell two 808s apart.
# Decay shape and knock dominate; absolute level barely registers once a track
# is gain-staged, so it is weighted low rather than excluded.
FEATURES = {
    "crestDb": 1.0,
    "attackMs": 0.6,
    "t20Ms": 1.4,
    "t40Ms": 1.0,
    "env100": 1.0,
    "env250": 1.2,
    "env500": 1.2,
    "env1000": 1.0,
    "env2000": 0.8,
    "h2": 1.3,
    "h3": 1.3,
    "h5": 1.1,
    "h7": 0.9,
    "punchDb": 1.4,
    "attackHfDb": 1.3,
    # dropSemitones and settleMs are REPORTED but deliberately not scored. The
    # drop is a fast sweep whose measured height depends on how the amplitude
    # envelope masks its start (correlation with the parameter is only +0.49
    # even tracked at F5), and a noisy feature in the metric steers the search
    # on noise. Every preset is instead GUARANTEED a real drop by the parameter
    # floors in optimise_bank.py - constructed, not measured.
    # rmsDb is NOT scored: the bank is deliberately level-matched, so loudness
    # is not available as a differentiator and counting it just made the metric
    # oscillate - optimise to zero clones, level-match, and 28 come back purely
    # because the levels converged. crestDb still carries the SHAPE of the level
    # (how far the peak stands above the average), which is what a listener
    # actually hears as punch.
}

# Harmonic ratios are heavy-tailed: a fold preset can sit 100x above a clean
# sub, which would swamp every other axis in a linear space.
LOG_FEATURES = {"h2", "h3", "h5", "h7", "t20Ms", "t40Ms", "attackMs"}


def normalise(rows):
    """Robust per-feature scaling (median / IQR), so one outlier preset cannot
    compress the rest of the bank into a single point."""
    scaled = []
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
        spread = max(q3 - q1, 1e-6)
        stats[feature] = (median, spread, values)

    for i, row in enumerate(rows):
        vector = {}
        for feature, (median, spread, values) in stats.items():
            vector[feature] = (values[i] - median) / spread
        scaled.append(vector)
    return scaled, stats


def distance(a, b):
    total = 0.0
    weight_total = 0.0
    for feature, weight in FEATURES.items():
        diff = a[feature] - b[feature]
        total += weight * diff * diff
        weight_total += weight
    return math.sqrt(total / weight_total)


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else "build/macos/preset_probe.json"
    rows = json.load(open(path))
    vectors, stats = normalise(rows)

    pairs = []
    for i in range(len(rows)):
        for j in range(i + 1, len(rows)):
            pairs.append((distance(vectors[i], vectors[j]), i, j))
    pairs.sort()

    print(f"=== {len(rows)} presets, {len(pairs)} pairs ===\n")

    print("--- 25 CLOSEST PAIRS (these are what 'sounds the same' means) ---")
    for d, i, j in pairs[:25]:
        same = "SAME-CAT" if rows[i]["category"] == rows[j]["category"] else "cross   "
        print(f"  {d:5.3f}  {same}  {rows[i]['name']:22s} [{rows[i]['category']:12s}]"
              f" <-> {rows[j]['name']:22s} [{rows[j]['category']}]")

    # Nearest-neighbour distance per preset: the honest "is this preset
    # redundant?" number. A bank is distinct when its WORST case is healthy.
    nearest = {}
    for d, i, j in pairs:
        if i not in nearest:
            nearest[i] = (d, j)
        if j not in nearest:
            nearest[j] = (d, i)

    values = sorted(v[0] for v in nearest.values())
    n = len(values)
    print(f"\n--- NEAREST-NEIGHBOUR DISTANCE DISTRIBUTION ---")
    print(f"  min    {values[0]:.3f}")
    print(f"  p10    {values[n // 10]:.3f}")
    print(f"  median {values[n // 2]:.3f}")
    print(f"  p90    {values[(9 * n) // 10]:.3f}")
    print(f"  max    {values[-1]:.3f}")

    clones = [(d, i, j) for i, (d, j) in nearest.items() if d < 0.35]
    print(f"\n  presets whose nearest neighbour is < 0.35 away: "
          f"{len(clones)}/{len(rows)}")

    # Per-category spread: a category whose members cluster tightly reads as
    # one sound with different names.
    print("\n--- WITHIN-CATEGORY MEAN PAIRWISE DISTANCE ---")
    by_cat = defaultdict(list)
    for idx, row in enumerate(rows):
        by_cat[row["category"]].append(idx)
    cat_scores = []
    for cat, members in by_cat.items():
        ds = [distance(vectors[a], vectors[b])
              for x, a in enumerate(members) for b in members[x + 1:]]
        cat_scores.append((sum(ds) / len(ds), min(ds), cat, len(members)))
    for mean, lo, cat, count in sorted(cat_scores):
        print(f"  {cat:14s} n={count:2d}  mean={mean:.3f}  closest-pair={lo:.3f}")

    # Raw feature ranges expose axes that are not doing any work.
    print("\n--- FEATURE SPREAD (raw units) ---")
    for feature in FEATURES:
        vals = sorted(float(r[feature]) for r in rows)
        print(f"  {feature:14s} min={vals[0]:10.3f}  med={vals[len(vals)//2]:10.3f}"
              f"  max={vals[-1]:10.3f}")

    hits = sum(1 for r in rows if r.get("ceilingHit"))
    print(f"\n  presets hitting the safety ceiling: {hits}/{len(rows)}")
    peaks = sorted(float(r["peakDb"]) for r in rows)
    print(f"  peak dBFS: min={peaks[0]:.2f}  median={peaks[len(peaks)//2]:.2f}  max={peaks[-1]:.2f}")
    rmss = sorted(float(r["rmsDb"]) for r in rows)
    print(f"  rms  dBFS: min={rmss[0]:.2f}  median={rmss[len(rmss)//2]:.2f}  max={rmss[-1]:.2f}")
    lands = sorted(abs(float(r["landingCents"])) for r in rows)
    print(f"  |landing cents|: median={lands[len(lands)//2]:.1f}  max={lands[-1]:.1f}")


if __name__ == "__main__":
    main()
