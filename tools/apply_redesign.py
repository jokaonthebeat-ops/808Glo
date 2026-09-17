#!/usr/bin/env python3
"""Apply a redesigned preset bank into generate_factory_presets.py's RECIPES.

Takes the JSON produced by the redesign pass, validates every parameter name and
value against PARAMETER_RANGES, and rewrites the RECIPES block in place. Refuses
to write anything if a single preset is malformed: a partially applied bank is
worse than none, because the result still passes the generator and only reveals
itself by ear.

  usage: apply_redesign.py <redesign.json> [--dry-run]
"""
import json
import re
import sys
from pathlib import Path

GENERATOR = Path(__file__).resolve().parent / "generate_factory_presets.py"

BOOL_PARAMS = {"legato"}
INT_PARAMS = {"voiceMode", "triggerMode", "driveMode", "bendRange"}

EXPECTED_COUNTS = {
    "Clean & Sub": 12, "Trap": 14, "Distorted": 13, "Drill": 13, "Detroit": 12,
    "West Coast": 12, "Long Glide": 12, "Short Punch": 12, "Experimental": 12,
    "Mix Ready": 16,
}

# Category invariants enforced by validate_presets.py. These are requirements of
# the CATEGORY, not design choices, so a proposal that misses one is repaired to
# the boundary rather than rejected - but every repair is printed, because a
# silent repair is how a bank drifts away from what was actually designed.
CATEGORY_INVARIANTS = {
    "Long Glide": {"voiceMode": ("==", 0), "legato": ("==", 1), "triggerMode": ("==", 1),
                   "glide": (">=", 350), "sustain": (">=", 0.6)},
    "Short Punch": {"triggerMode": ("==", 0), "decay": ("<=", 600), "release": ("<=", 120)},
    "Drill": {"voiceMode": ("==", 0), "legato": ("==", 1), "glide": (">=", 80)},
    "Distorted": {"drive": (">=", 16)},
    "Clean & Sub": {"drive": ("<=", 2)},
}


def repair_category(category, name, overrides, repairs, inherited):
    """Force the category invariants, then the cross-cutting legato rule.

    Checks the EFFECTIVE value the generator will emit (defaults, then the
    category base, then this preset's overrides). Judging the override alone
    reports repairs for presets that already satisfy the rule by inheritance.
    """
    def effective(param, fallback=0):
        if param in overrides:
            return float(overrides[param])
        if param in inherited:
            return float(inherited[param])
        return float(fallback)

    for param, (operator, bound) in CATEGORY_INVARIANTS.get(category, {}).items():
        value = effective(param)
        if operator == "==" and value != bound:
            overrides[param] = bound
            repairs.append(f"{category}/{name}: {param} {value} -> {bound} (category rule)")
        elif operator == ">=" and value < bound:
            overrides[param] = bound
            repairs.append(f"{category}/{name}: {param} {value} -> {bound} (category minimum)")
        elif operator == "<=" and value > bound:
            overrides[param] = bound
            repairs.append(f"{category}/{name}: {param} {value} -> {bound} (category maximum)")

    # legato anywhere requires mono and a nonzero glide.
    if effective("legato") >= 0.5:
        if effective("voiceMode") != 0:
            overrides["voiceMode"] = 0
            repairs.append(f"{category}/{name}: voiceMode -> 0 (legato requires mono)")
        if effective("glide") <= 0:
            overrides["glide"] = 80
            repairs.append(f"{category}/{name}: glide -> 80 (legato requires nonzero glide)")


def load_generator():
    """Import the generator so ranges, defaults and category bases come from the
    single source of truth rather than a second copy that can drift."""
    import importlib.util
    spec = importlib.util.spec_from_file_location("_gen", GENERATOR)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    ranges = {k: (float(lo), float(hi)) for k, (lo, hi) in module.PARAMETER_RANGES.items()}
    inherited = {}
    for category in EXPECTED_COUNTS:
        merged = dict(module.DEFAULT_PARAMETERS)
        merged.update(module.CATEGORY_BASES.get(category, {}))
        inherited[category] = {k: (1 if v is True else 0 if v is False else v)
                               for k, v in merged.items()}
    return ranges, inherited


def format_value(param, value):
    if param in BOOL_PARAMS:
        return "True" if float(value) >= 0.5 else "False"
    if param in INT_PARAMS or float(value) == int(float(value)):
        return str(int(float(value)))
    return repr(round(float(value), 4))


def escape(text):
    return text.replace("\\", "\\\\").replace('"', '\\"')


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    payload = json.load(open(sys.argv[1]))
    dry_run = "--dry-run" in sys.argv
    ranges, inherited = load_generator()

    categories = payload["categories"] if isinstance(payload, dict) else payload
    errors = []
    repairs = []
    by_category = {}
    seen_names = {}

    for entry in categories:
        proposal = entry.get("proposal") or entry
        category = proposal["category"]
        presets = proposal["presets"]

        expected = EXPECTED_COUNTS.get(category)
        if expected is None:
            errors.append(f"unknown category {category!r}")
            continue
        if len(presets) != expected:
            errors.append(f"{category}: {len(presets)} presets, expected {expected}")

        cleaned = []
        for preset in presets:
            name = preset["name"].strip()
            if name in seen_names:
                errors.append(f"{category}: duplicate preset name {name!r} (also in {seen_names[name]})")
            seen_names[name] = category

            overrides = {}
            for item in preset["overrides"]:
                param, value = item["param"], item["value"]
                if param not in ranges:
                    errors.append(f"{category}/{name}: unknown parameter {param!r}")
                    continue
                lo, hi = ranges[param]
                if not (lo <= float(value) <= hi):
                    errors.append(
                        f"{category}/{name}: {param}={value} outside legal range [{lo}, {hi}]")
                    continue
                overrides[param] = value
            if not overrides:
                errors.append(f"{category}/{name}: no usable overrides")
            repair_category(category, name, overrides, repairs, inherited.get(category, {}))
            cleaned.append({
                "name": name,
                "description": preset["description"].strip(),
                "tags": [t.strip().lower().replace(" ", "-") for t in preset["tags"]][:4],
                "overrides": overrides,
            })
        by_category[category] = cleaned

    missing = set(EXPECTED_COUNTS) - set(by_category)
    if missing:
        errors.append(f"missing categories: {sorted(missing)}")

    if errors:
        print(f"REFUSING TO WRITE - {len(errors)} problem(s):")
        for error in errors[:40]:
            print(f"  {error}")
        if len(errors) > 40:
            print(f"  ... and {len(errors) - 40} more")
        return 1

    lines = ['RECIPES: "OrderedDict[str, list[dict[str, Any]]]" = OrderedDict(', "    ["]
    for category in EXPECTED_COUNTS:
        lines.append(f'        ("{escape(category)}", [')
        for preset in by_category[category]:
            overrides = ", ".join(
                f"{param}={format_value(param, value)}"
                for param, value in preset["overrides"].items())
            tags = ", ".join(f'"{escape(t)}"' for t in preset["tags"])
            lines.append(
                f'            R("{escape(preset["name"])}", '
                f'"{escape(preset["description"])}", [{tags}], {overrides}),')
        lines.append("        ]),")
    lines.append("    ]")
    lines.append(")")
    new_block = "\n".join(lines)

    source = GENERATOR.read_text()
    start = source.index('RECIPES: "OrderedDict[str, list[dict[str, Any]]]" = OrderedDict(')
    end = source.index("\ndef unique_tags", start)
    updated = source[:start] + new_block + "\n\n" + source[end:].lstrip("\n")

    if repairs:
        print(f"{len(repairs)} category-invariant repair(s):")
        for repair in repairs:
            print(f"  {repair}")

    total = sum(len(v) for v in by_category.values())
    if dry_run:
        print(f"OK - {total} presets would be written across {len(by_category)} categories")
        return 0

    GENERATOR.write_text(updated)
    print(f"wrote {total} presets across {len(by_category)} categories into {GENERATOR.name}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
