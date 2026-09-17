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


def load_ranges():
    source = GENERATOR.read_text()
    block = re.search(r"PARAMETER_RANGES.*?OrderedDict\(\s*\[(.*?)\]\s*\)", source, re.S).group(1)
    ranges = {}
    for name, lo, hi in re.findall(r'\("(\w+)",\s*\(([-\d.]+),\s*([-\d.]+)\)\)', block):
        ranges[name] = (float(lo), float(hi))
    return ranges


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
    ranges = load_ranges()

    categories = payload["categories"] if isinstance(payload, dict) else payload
    errors = []
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

    total = sum(len(v) for v in by_category.values())
    if dry_run:
        print(f"OK - {total} presets would be written across {len(by_category)} categories")
        return 0

    GENERATOR.write_text(updated)
    print(f"wrote {total} presets across {len(by_category)} categories into {GENERATOR.name}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
