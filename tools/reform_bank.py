#!/usr/bin/env python3
"""Reform the factory bank on the axes that actually reach the output at 43 Hz.

Two measured facts drive this:

1. PUNCH. Envelope.h computes decay as `decayStart + (sustain - decayStart) *
   progress^ampCurve`. With ampCurve > 1 that is a plateau-then-cliff: at the
   bank median of 1.65 the level is only 0.93 dB down a quarter of the way
   through the decay. All 128 presets sit between 1.15 and 2.30, so not one of
   them decays like something struck, and the body never gets out of the
   transient's way.

2. DISTINCTNESS. A first redesign pass fixed (1) by giving every preset a low
   ampCurve, and MEASURED WORSE: bank clones 82 -> 84, Trap's within-category
   median distance collapsed 0.747 -> 0.440. Applying one fix uniformly makes a
   category converge. So strike character has to be spread ACROSS each category,
   not pinned - which also revives the axis: ampCurve scores 1.63 over its full
   range but only 0.364 over the span the bank currently uses.

The spread is assigned by golden-ratio index rather than sorted by decay, so
ampCurve stays decorrelated from decay and presets fill the (length x strike)
plane instead of lining up along its diagonal. Decay is the single
highest-authority axis and strike is the one being revived; a producer chooses
between 808s on exactly those two questions - how long does it ring, and how
hard does it hit - so this is a musical grid, not a metric-chasing one.
"""
import importlib.util
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
GENERATOR = ROOT / "tools" / "generate_factory_presets.py"

# Per-category strike range. Wide where the genre genuinely spans knocky to
# boomy, narrower where the category's job constrains it. Ranges overlap between
# categories on purpose: categories are separated by drive, harmonics, glide and
# decay band, while ampCurve does its work WITHIN a category.
STRIKE_RANGES = {
    "Short Punch": (0.28, 0.62),
    "Detroit": (0.30, 0.72),
    "Trap": (0.30, 0.98),
    "Drill": (0.34, 1.02),
    "Distorted": (0.30, 0.92),
    "Mix Ready": (0.34, 1.12),
    "West Coast": (0.44, 1.18),
    "Clean & Sub": (0.40, 1.28),
    "Experimental": (0.28, 1.45),
    "Long Glide": (0.58, 1.62),
}

GOLDEN = 0.6180339887498949


def load_generator():
    spec = importlib.util.spec_from_file_location("_gen", GENERATOR)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main():
    gen = load_generator()
    dry_run = "--dry-run" in sys.argv

    out = []
    changes = []
    for category, recipes in gen.RECIPES.items():
        lo, hi = STRIKE_RANGES[category]
        presets = []
        for index, recipe in enumerate(recipes):
            overrides = dict(recipe["overrides"])

            # Golden-ratio position: a distinct value at any bank size, and no
            # alignment with the order presets happen to be written in. A plain
            # (index * k) % m spread once gave a sibling project eleven distinct
            # values for sixteen presets, making presets eleven apart identical.
            position = (index * GOLDEN) % 1.0
            strike = round(lo + (hi - lo) * position, 3)

            previous = overrides.get("ampCurve", gen.CATEGORY_BASES.get(category, {}).get(
                "ampCurve", gen.DEFAULT_PARAMETERS["ampCurve"]))
            overrides["ampCurve"] = strike
            changes.append((category, recipe["name"], previous, strike))

            presets.append({
                "name": recipe["name"],
                "description": recipe["description"],
                "tags": recipe["tags"],
                "identity": "strike-spread reform",
                "separatedFrom": "ampCurve spread within category",
                "overrides": [{"param": k, "value": (1.0 if v is True else 0.0 if v is False else float(v))}
                              for k, v in overrides.items()],
            })
        out.append({"proposal": {"category": category, "territory": "reform",
                                 "approach": "strike spread", "presets": presets}})

    strikes = sorted(c[3] for c in changes)
    print(f"ampCurve: was 1.15-2.30 across all 128 presets (plateau side of 1.0)")
    print(f"          now {strikes[0]:.2f}-{strikes[-1]:.2f}, median {strikes[len(strikes)//2]:.2f}")
    for category in STRIKE_RANGES:
        values = sorted(c[3] for c in changes if c[0] == category)
        print(f"  {category:14s} {values[0]:.2f} .. {values[-1]:.2f}  ({len(set(values))} distinct)")

    target = ROOT / "build" / "macos" / "reform.json"
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(json.dumps({"categories": out}, indent=1))
    print(f"\nwrote {target}")
    if dry_run:
        print("(dry run - apply with tools/apply_redesign.py)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
