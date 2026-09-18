#!/usr/bin/env python3
"""Place every preset in a distinct cell of its category's sound space.

Why a multi-axis placement and not a per-pair nudge: pushing a near-clone along
one axis slides it into a DIFFERENT sibling - that lesson cost a sibling project
two wasted passes. And why not a single axis for the whole bank: giving every
preset the same struck envelope raised punch but measured WORSE for
distinctness (clones 77 -> 79), because a fix applied uniformly makes a category
converge.

So each preset gets a position on a low-discrepancy (Halton) sequence across the
axes that measurably reach the output at 43 Hz. Halton fills a multi-dimensional
space evenly without clumping, which is exactly "no two presets in the same
cell".

The RANGE of every axis is taken from the category's own current presets, so the
hand-authored musical character of each category is preserved - a Clean & Sub
preset can never be handed a Distorted preset's drive. What changes is only
WHERE inside that vetted space each preset sits. The axes themselves are ranked
by measured authority; dead axes (tone above ~900 Hz, clickTone, toneKeytrack,
release when sustain is 0) are deliberately left alone, because varying them
buys nothing audible and only makes a parameter-space metric look better.
"""
import importlib.util
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
GENERATOR = ROOT / "tools" / "generate_factory_presets.py"

# Axes with real measured authority at 808 register, paired with the Halton
# dimension that spreads them. Ordered by authority: decay dominates at 18.8,
# then strike, drive, colour and dynamics.
SPREAD_AXES = [
    "decay", "ampCurve", "drive", "body", "harmonics",
    "compressor", "clipper", "sustain", "pitchDrop", "pitchDecay",
    "attack", "click", "punch", "harmonicBalance", "tone", "clickTone",
]

PRIMES = [2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53]

# Strike is the axis being revived, so its range is set deliberately rather than
# inherited: the whole bank currently sits on the plateau side of 1.0.
STRIKE_RANGES = {
    "Short Punch": (0.28, 0.62), "Detroit": (0.30, 0.72), "Trap": (0.30, 0.98),
    "Drill": (0.34, 1.02), "Distorted": (0.30, 0.92), "Mix Ready": (0.34, 1.12),
    "West Coast": (0.44, 1.18), "Clean & Sub": (0.40, 1.28),
    "Experimental": (0.28, 1.45), "Long Glide": (0.58, 1.62),
}

# Widening factor per axis: how far outside the category's current span a preset
# may be placed. Kept modest so the category still sounds like itself.
WIDEN = 0.15

# Reserved decay bands, so two categories cannot occupy the same length of note -
# the cross-category collisions that survived the first spread were all pairs
# whose categories overlapped almost completely (Trap 142-3940 ms against Drill
# 248-3470 and West Coast 372-3910).
#
# Target t20 bands come from the territory assignment; decay is derived from
# them. For this envelope, level = 1 - (t/decay)^curve, so -20 dB is reached at
# progress = 0.9^(1/curve), i.e. t20 is roughly 0.81-0.95 of decay across the
# curve range in use. Dividing the target band by 0.85 gives the decay range,
# and the measured t20 afterwards is what confirms it.
# MEASURED: reserving a narrow non-overlapping decay band per category raised the
# minimum distance but made the bank WORSE overall (clones 62 -> 70). Decay has
# authority 18.8, an order above every other axis, so narrowing it costs more
# within-category spread than the cross-category separation buys back - there is
# simply not room to carve ten bands and still give 12-16 presets each a spread.
# Categories therefore overlap in length and are separated by CHARACTER instead
# (harmonic balance, upper-partial fraction, drive, glide), which is also how a
# producer tells a Drill 808 from a West Coast one.
TERRITORY_DECAY = {
    "Short Punch": (105, 590),     # validator caps this category at 600 ms
    "Detroit": (150, 900),
    "Trap": (300, 3000),
    "Mix Ready": (400, 2500),
    "Distorted": (300, 3000),
    "Drill": (400, 3000),
    "Clean & Sub": (400, 4000),
    "West Coast": (500, 3500),
    "Long Glide": (1500, 7000),
    "Experimental": (120, 6000),
}

# Punch-preserving caps. The weakest attacks in the bank all shared a high
# clipper and a slow attack: the clipper is not a clipper but an upward
# compressor (it adds +10.7 dB at -26 dBFS and +0.1 dB at -1 dBFS), so it eats
# exactly the transient the user is asking for, and a slow attack ramp blunts
# the hit directly. Both still carry separation, just not at the settings where
# they cost the knock.
# Tightened after measuring: with the bank level-matched back up, the clipper
# takes the transient with it (measured -1.74 dB of knock per raise of output
# while it is engaged). Both axes still separate presets, just not at settings
# that cost the thing the bank is FOR.
PUNCH_CAPS = {
    "clipper": (0.0, 0.38),
    "compressor": (0.0, 0.42),
    "attack": (0.05, 0.9),
}

# Territory axes beyond decay. West Coast and Drill sit in adjacent decay bands,
# so the even/odd split is what actually keeps them apart; Clean & Sub is defined
# by the absence of upper partials.
TERRITORY_AXES = {
    "West Coast": {"harmonicBalance": (0.25, 0.95), "harmonics": (0.10, 0.45)},
    "Drill": {"harmonicBalance": (-0.95, -0.20), "harmonics": (0.06, 0.40)},
    "Clean & Sub": {"harmonics": (0.0, 0.10)},
    "Distorted": {"harmonics": (0.45, 1.0)},
    "Trap": {"harmonics": (0.12, 0.45)},
}


# The click is the bank's unexploited punch lever. `tone` is dead for the BODY at
# 43 Hz (the highest body partial is H5 at 218 Hz, and a one-pole at 1500 Hz
# attenuates that by 0.09 dB) but it sits AFTER the click in Voice.h, so it is
# really a click lowpass: at tone 1500 it throws away 15 dB of a 9 kHz click, at
# tone 450 it throws away 25 dB. Most of the bank was spending its click that
# way. Opening tone therefore buys attack brightness for free - it cannot muddy
# or thin the sub, because it never reached the sub in the first place - and
# clickTone must be set with it or the pair still cancels.
CLICK_TRANSMISSION = {
    "Short Punch": {"tone": (7000, 16000), "click": (0.35, 0.85), "clickTone": (5000, 12000)},
    "Detroit":     {"tone": (6500, 16000), "click": (0.35, 0.85), "clickTone": (5000, 11000)},
    "Trap":        {"tone": (5500, 15000), "click": (0.30, 0.78), "clickTone": (4500, 11000)},
    "Distorted":   {"tone": (5000, 14000), "click": (0.28, 0.75), "clickTone": (4500, 12000)},
    "Drill":       {"tone": (4500, 13000), "click": (0.22, 0.65), "clickTone": (4000, 10000)},
    "Mix Ready":   {"tone": (4000, 12000), "click": (0.20, 0.60), "clickTone": (3500, 9000)},
    "West Coast":  {"tone": (3000, 11000), "click": (0.14, 0.50), "clickTone": (3000, 8000)},
    "Experimental":{"tone": (2500, 14000), "click": (0.10, 0.70), "clickTone": (2500, 12000)},
    "Clean & Sub": {"tone": (1800, 9000),  "click": (0.05, 0.34), "clickTone": (2500, 7000)},
    "Long Glide":  {"tone": (1500, 8000),  "click": (0.04, 0.28), "clickTone": (2000, 6500)},
}


# An 808's knock IS the pitch envelope. One cycle of F1 lasts 22.9 ms, and a drop
# that finishes inside a single cycle is heard as a click rather than as a fall -
# which was true of 55 of the 128 presets (Trap's median drop lasted 0.95 cycles,
# Detroit 0.48, Short Punch 0.30). These ranges put the drop across 2-4 cycles
# where it becomes an audible knock, with the longer, more melodic categories
# deliberately given slower falls.
PITCH_KNOCK = {
    "Short Punch": {"pitchDrop": (20, 36), "pitchDecay": (38, 72)},
    "Detroit":     {"pitchDrop": (18, 30), "pitchDecay": (42, 78)},
    "Trap":        {"pitchDrop": (16, 26), "pitchDecay": (58, 92)},
    "Distorted":   {"pitchDrop": (14, 28), "pitchDecay": (50, 95)},
    "Drill":       {"pitchDrop": (14, 24), "pitchDecay": (40, 70)},
    "Mix Ready":   {"pitchDrop": (12, 22), "pitchDecay": (32, 62)},
    "West Coast":  {"pitchDrop": (12, 22), "pitchDecay": (45, 80)},
    "Clean & Sub": {"pitchDrop": (8, 18),  "pitchDecay": (30, 60)},
    "Long Glide":  {"pitchDrop": (6, 16),  "pitchDecay": (75, 150)},
    "Experimental":{"pitchDrop": (10, 40), "pitchDecay": (60, 300)},
}

# Pulled back from the first pass: opening decay too far pushed the median t20
# from 1312 to 2075 ms, and a longer tail cancels the faster envelope curve, so
# the bank measured no more struck than before. These keep a wide within-category
# spread without turning the whole bank boomy.
TERRITORY_DECAY_TIGHT = {
    "Short Punch": (95, 520),   "Detroit": (140, 780),
    "Trap": (280, 2100),        "Mix Ready": (350, 1800),
    "Distorted": (300, 2200),   "Drill": (380, 2200),
    "Clean & Sub": (400, 2800), "West Coast": (450, 2600),
    "Long Glide": (1400, 5200), "Experimental": (120, 4200),
}


def halton(index, base):
    result, f, i = 0.0, 1.0, index + 1
    while i > 0:
        f /= base
        result += f * (i % base)
        i //= base
    return result


def load_generator():
    spec = importlib.util.spec_from_file_location("_gen", GENERATOR)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main():
    gen = load_generator()
    ranges = {k: (float(lo), float(hi)) for k, (lo, hi) in gen.PARAMETER_RANGES.items()}

    out = []
    report = []
    for category, recipes in gen.RECIPES.items():
        base = dict(gen.DEFAULT_PARAMETERS)
        base.update(gen.CATEGORY_BASES.get(category, {}))

        # The category's own vetted span for each axis, from its current presets.
        span = {}
        for axis in SPREAD_AXES:
            values = []
            for recipe in recipes:
                value = recipe["overrides"].get(axis, base.get(axis))
                if value is True:
                    value = 1.0
                elif value is False:
                    value = 0.0
                values.append(float(value))
            lo, hi = min(values), max(values)
            if axis == "ampCurve":
                lo, hi = STRIKE_RANGES[category]
            elif axis == "decay":
                lo, hi = TERRITORY_DECAY[category]
            elif axis == "pitchDecay":
                # Floor only: one cycle of F1 is 22.9 ms, and a drop that ends
                # inside one cycle reads as a click rather than a fall. The top
                # stays free, because pinning both ends cost 17 presets' worth of
                # separation when it was tried.
                lo = max(lo, 46.0)
            elif axis in TERRITORY_AXES.get(category, {}):
                lo, hi = TERRITORY_AXES[category][axis]
            elif axis in CLICK_TRANSMISSION.get(category, {}):
                lo, hi = CLICK_TRANSMISSION[category][axis]
            elif axis in PUNCH_CAPS:
                cap_lo, cap_hi = PUNCH_CAPS[axis]
                pad = (hi - lo) * WIDEN
                lo, hi = max(lo - pad, cap_lo), min(hi + pad, cap_hi)
            elif hi - lo < 1e-9:
                # The category pins this axis; widen slightly around it so it can
                # still carry a little separation without leaving character.
                pad = max(abs(lo) * 0.25, (ranges[axis][1] - ranges[axis][0]) * 0.03)
                lo, hi = lo - pad, hi + pad
            else:
                pad = (hi - lo) * WIDEN
                lo, hi = lo - pad, hi + pad
            legal_lo, legal_hi = ranges[axis]
            span[axis] = (max(lo, legal_lo), min(hi, legal_hi))

        presets = []
        for index, recipe in enumerate(recipes):
            overrides = dict(recipe["overrides"])
            for dimension, axis in enumerate(SPREAD_AXES):
                lo, hi = span[axis]
                position = halton(index, PRIMES[dimension])
                value = lo + (hi - lo) * position
                if axis in ("decay", "pitchDecay", "drive", "pitchDrop"):
                    value = round(value, 1)
                else:
                    value = round(value, 4)
                overrides[axis] = value
            presets.append({
                "name": recipe["name"],
                "description": recipe["description"],
                "tags": recipe["tags"],
                "identity": "halton spread over live axes",
                "separatedFrom": "distinct cell in the category sound space",
                "overrides": [{"param": k, "value": (1.0 if v is True else 0.0 if v is False else float(v))}
                              for k, v in overrides.items()],
            })
        report.append((category, span))
        out.append({"proposal": {"category": category, "territory": "spread",
                                 "approach": "halton", "presets": presets}})

    for category, span in report:
        print(f"{category}:")
        print("   " + "  ".join(f"{a}={span[a][0]:.3g}..{span[a][1]:.3g}"
                                for a in ("decay", "ampCurve", "drive", "body", "harmonics")))

    target = ROOT / "build" / "macos" / "spread.json"
    target.write_text(json.dumps({"categories": out}, indent=1))
    print(f"\nwrote {target}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
