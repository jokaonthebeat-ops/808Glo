#!/usr/bin/env python3
"""Strict validation for the 808Glo Pro factory preset resource."""

from __future__ import annotations

import argparse
import json
import math
import re
import sys
from collections import Counter
from pathlib import Path
from typing import Any


SCHEMA_VERSION = 1
EXPECTED_AUTHOR = "Diamond Loopz"
EXPECTED_BANK_NAME = "808Glo Pro Factory Bank"
EXPECTED_COUNT = 128
ROOT = Path(__file__).resolve().parents[1]
DEFAULT_BANK_PATH = ROOT / "Resources" / "FactoryPresets.json"

EXPECTED_CATEGORY_COUNTS = {
    "Clean & Sub": 12,
    "Trap": 14,
    "Distorted": 13,
    "Drill": 13,
    "Detroit": 12,
    "West Coast": 12,
    "Long Glide": 12,
    "Short Punch": 12,
    "Experimental": 12,
    "Mix Ready": 16,
}

PARAMETER_RANGES: dict[str, tuple[float, float]] = {
    "voiceMode": (0, 1),
    "legato": (0, 1),
    "triggerMode": (0, 1),
    "velocitySens": (0, 100),
    "tune": (-24, 24),
    "fine": (-100, 100),
    "bendRange": (1, 24),
    "glide": (0, 2000),
    "body": (0, 1),
    "harmonics": (0, 1),
    "harmonicBalance": (-1, 1),
    "pitchDrop": (0, 48),
    "pitchDecay": (5, 500),
    "pitchCurve": (0.25, 4),
    "attack": (0.05, 50),
    "hold": (0, 250),
    "decay": (50, 8000),
    "sustain": (0, 1),
    "release": (5, 3000),
    "ampCurve": (0.25, 4),
    "click": (0, 1),
    "clickTone": (500, 16000),
    "clickDecay": (0.1, 30),
    "punch": (0, 1),
    "tone": (45, 18000),
    "toneKeytrack": (0, 1),
    "drive": (0, 36),
    "driveMode": (0, 3),
    "compressor": (0, 1),
    "clipper": (0, 1),
    "output": (-24, 12),
}

# Raw-value intervals from Source/ParameterLayout.cpp. A zero interval means
# JUCE leaves the raw value continuous; nonzero intervals are snapped by
# NormalisableRange::snapToLegalValue(). Choices and bools are discrete too,
# although bool type validation is handled separately below.
PARAMETER_INTERVALS: dict[str, float] = {
    "voiceMode": 1,
    "legato": 1,
    "triggerMode": 1,
    "velocitySens": 0.01,
    "tune": 1,
    "fine": 0.1,
    "bendRange": 1,
    "glide": 0,
    "body": 0.0001,
    "harmonics": 0.0001,
    "harmonicBalance": 0.0001,
    "pitchDrop": 0.01,
    "pitchDecay": 0,
    "pitchCurve": 0.0001,
    "attack": 0,
    "hold": 0,
    "decay": 0,
    "sustain": 0.0001,
    "release": 0,
    "ampCurve": 0.0001,
    "click": 0.0001,
    "clickTone": 0,
    "clickDecay": 0,
    "punch": 0.0001,
    "tone": 0,
    "toneKeytrack": 0.0001,
    "drive": 0.01,
    "driveMode": 1,
    "compressor": 0.0001,
    "clipper": 0.0001,
    "output": 0.01,
}

INTEGER_PARAMETERS = {"voiceMode", "triggerMode", "tune", "bendRange", "driveMode"}
ENUM_VALUES = {
    "voiceMode": {0, 1},
    "triggerMode": {0, 1},
    "driveMode": {0, 1, 2, 3},
}

# Minimum changes treated as a clearly intentional sound-design difference.
# The validator requires every pair to differ in at least two of these audible
# dimensions, preventing a bank padded with renamed or gain-only variants.
MEANINGFUL_THRESHOLDS: dict[str, float] = {
    "glide": 25,
    "body": 0.025,
    "harmonics": 0.025,
    "harmonicBalance": 0.08,
    "pitchDrop": 1.5,
    "pitchDecay": 7,
    "pitchCurve": 0.08,
    "attack": 0.25,
    "hold": 3,
    "decay": 45,
    "sustain": 0.025,
    "release": 8,
    "ampCurve": 0.08,
    "click": 0.025,
    "clickTone": 300,
    "clickDecay": 0.25,
    "punch": 0.025,
    "tone": 180,
    "toneKeytrack": 0.025,
    "drive": 0.75,
    "compressor": 0.025,
    "clipper": 0.025,
}

CATEGORICAL_SOUND_PARAMETERS = {"voiceMode", "legato", "triggerMode", "driveMode"}
NON_SOUND_PARAMETERS = {"velocitySens", "tune", "fine", "bendRange", "output"}
NAME_PATTERN = re.compile(r"^[A-Za-z0-9][A-Za-z0-9 &'’-]*[A-Za-z0-9]$")
TAG_PATTERN = re.compile(r"^[a-z0-9]+(?:-[a-z0-9]+)*$")


class ValidationErrors:
    def __init__(self) -> None:
        self.messages: list[str] = []

    def add(self, message: str) -> None:
        self.messages.append(message)

    def require(self, condition: bool, message: str) -> None:
        if not condition:
            self.add(message)


def is_number(value: Any) -> bool:
    return isinstance(value, (int, float)) and not isinstance(value, bool)


def snap_to_legal_value(
    value: float,
    minimum: float,
    maximum: float,
    interval: float,
) -> float:
    """Mirror JUCE NormalisableRange raw-value snapping for validation."""
    constrained = min(max(value, minimum), maximum)
    if interval <= 0:
        return constrained
    snapped = minimum + interval * math.floor((constrained - minimum) / interval + 0.5)
    return min(max(snapped, minimum), maximum)


def sound_difference_count(first: dict[str, Any], second: dict[str, Any]) -> int:
    count = 0
    for parameter_id in CATEGORICAL_SOUND_PARAMETERS:
        if first[parameter_id] != second[parameter_id]:
            count += 1

    for parameter_id, threshold in MEANINGFUL_THRESHOLDS.items():
        if abs(float(first[parameter_id]) - float(second[parameter_id])) >= threshold:
            count += 1

    return count


def validate_semantics(
    preset: dict[str, Any],
    parameters: dict[str, Any],
    index: int,
    errors: ValidationErrors,
) -> None:
    label = f"preset[{index}] {preset.get('name', '<unnamed>')!r}"
    category = preset.get("category")

    if parameters["legato"]:
        errors.require(parameters["voiceMode"] == 0, f"{label}: legato requires mono voiceMode")
        errors.require(parameters["glide"] > 0, f"{label}: legato requires nonzero glide")

    if category == "Long Glide":
        errors.require(parameters["voiceMode"] == 0, f"{label}: Long Glide must be mono")
        errors.require(parameters["legato"] is True, f"{label}: Long Glide must enable legato")
        errors.require(parameters["triggerMode"] == 1, f"{label}: Long Glide must use gate trigger mode")
        errors.require(parameters["glide"] >= 350, f"{label}: Long Glide must use at least 350 ms glide")
        errors.require(parameters["sustain"] >= 0.6, f"{label}: Long Glide must retain at least 60% sustain")

    if category == "Short Punch":
        errors.require(parameters["triggerMode"] == 0, f"{label}: Short Punch must use one-shot trigger mode")
        errors.require(parameters["decay"] <= 600, f"{label}: Short Punch decay exceeds 600 ms")
        errors.require(parameters["release"] <= 120, f"{label}: Short Punch release exceeds 120 ms")

    if category == "Drill":
        errors.require(parameters["voiceMode"] == 0, f"{label}: Drill must be mono")
        errors.require(parameters["legato"] is True, f"{label}: Drill must enable legato")
        errors.require(parameters["glide"] >= 80, f"{label}: Drill glide is too short")

    if category == "Distorted":
        errors.require(parameters["drive"] >= 16, f"{label}: Distorted preset lacks meaningful drive")

    if category == "Clean & Sub":
        errors.require(parameters["drive"] <= 2, f"{label}: Clean & Sub drive exceeds 2 dB")


def validate_tag_semantics(
    preset: dict[str, Any],
    parameters: dict[str, Any],
    tags: set[str],
    index: int,
    errors: ValidationErrors,
) -> None:
    label = f"preset[{index}] {preset.get('name', '<unnamed>')!r}"

    if "mono" in tags:
        errors.require(parameters["voiceMode"] == 0, f"{label}: mono tag requires mono voiceMode")
    if "mono-legato" in tags:
        errors.require(parameters["voiceMode"] == 0, f"{label}: mono-legato tag requires mono voiceMode")
        errors.require(parameters["legato"] is True, f"{label}: mono-legato tag requires legato")
        errors.require(parameters["glide"] > 0, f"{label}: mono-legato tag requires nonzero glide")
    if "poly" in tags or "polyphonic" in tags:
        errors.require(parameters["voiceMode"] == 1, f"{label}: poly tag requires poly voiceMode")
    if "legato" in tags:
        errors.require(parameters["legato"] is True, f"{label}: legato tag requires legato")
    glide_tags = sorted(tag for tag in tags if "glide" in tag.split("-"))
    if glide_tags:
        errors.require(
            parameters["glide"] > 0,
            f"{label}: glide tag(s) {glide_tags} require nonzero glide",
        )


def validate_bank(path: Path) -> tuple[list[str], Counter[str], int]:
    errors = ValidationErrors()

    try:
        document = json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError:
        return ([f"Preset resource does not exist: {path}"], Counter(), 0)
    except (OSError, UnicodeError, ValueError, RecursionError) as error:
        return ([f"Could not parse {path}: {error}"], Counter(), 0)

    if not isinstance(document, dict):
        return (["Bank root must be a JSON object"], Counter(), 0)

    expected_root_metadata = {"schemaVersion", "bankName", "author", "presetCount", "presets"}
    actual_root_metadata = set(document)
    errors.require(
        actual_root_metadata == expected_root_metadata,
        f"Root keys differ; missing={sorted(expected_root_metadata - actual_root_metadata)}, "
        f"unknown={sorted(actual_root_metadata - expected_root_metadata)}",
    )
    root_schema_version = document.get("schemaVersion")
    errors.require(
        type(root_schema_version) is int and root_schema_version == SCHEMA_VERSION,
        "Root schemaVersion must be the integer 1",
    )
    errors.require(document.get("bankName") == EXPECTED_BANK_NAME, f"Root bankName must be {EXPECTED_BANK_NAME!r}")
    errors.require(document.get("author") == EXPECTED_AUTHOR, "Root author must be Diamond Loopz")
    root_preset_count = document.get("presetCount")
    errors.require(
        type(root_preset_count) is int and root_preset_count == EXPECTED_COUNT,
        "Root presetCount must be the integer 128",
    )

    presets = document.get("presets")
    if not isinstance(presets, list):
        errors.add("Root presets value must be an array")
        return (errors.messages, Counter(), 0)

    errors.require(len(presets) == EXPECTED_COUNT, f"Expected exactly {EXPECTED_COUNT} presets, found {len(presets)}")

    names: set[str] = set()
    descriptions: set[str] = set()
    categories: Counter[str] = Counter()
    parameter_sets: list[tuple[str, dict[str, Any]]] = []
    full_vectors: set[tuple[Any, ...]] = set()
    sound_vectors: set[tuple[Any, ...]] = set()

    expected_metadata = {
        "schemaVersion", "name", "category", "description", "tags", "author", "parameters"
    }

    for index, preset in enumerate(presets):
        if not isinstance(preset, dict):
            errors.add(f"preset[{index}] must be an object")
            continue

        label = f"preset[{index}]"
        actual_metadata = set(preset)
        errors.require(
            actual_metadata == expected_metadata,
            f"{label}: metadata keys differ; missing={sorted(expected_metadata - actual_metadata)}, "
            f"unknown={sorted(actual_metadata - expected_metadata)}",
        )
        preset_schema_version = preset.get("schemaVersion")
        errors.require(
            type(preset_schema_version) is int and preset_schema_version == SCHEMA_VERSION,
            f"{label}: schemaVersion must be the integer 1",
        )
        errors.require(preset.get("author") == EXPECTED_AUTHOR, f"{label}: author must be Diamond Loopz")

        name = preset.get("name")
        if not isinstance(name, str) or not name.strip():
            errors.add(f"{label}: name must be a non-empty string")
            name = f"<unnamed-{index}>"
        else:
            errors.require(name == name.strip(), f"{label}: name has leading or trailing whitespace")
            errors.require(bool(NAME_PATTERN.fullmatch(name)), f"{label}: invalid preset name {name!r}")
            folded_name = name.casefold()
            errors.require(folded_name not in names, f"{label}: duplicate name {name!r}")
            names.add(folded_name)

        category = preset.get("category")
        if not isinstance(category, str) or category not in EXPECTED_CATEGORY_COUNTS:
            errors.add(f"{label} {name!r}: unknown category {category!r}")
        else:
            categories[category] += 1
            forbidden_prefixes = (f"{category} - ", f"{category}: ", f"{category} – ")
            errors.require(not name.startswith(forbidden_prefixes), f"{label}: name contains category prefix")

        description = preset.get("description")
        if not isinstance(description, str) or len(description.strip()) < 24:
            errors.add(f"{label} {name!r}: description must contain at least 24 characters")
        else:
            errors.require(description[-1] in ".!?", f"{label} {name!r}: description must end with punctuation")
            folded_description = description.casefold()
            errors.require(folded_description not in descriptions, f"{label} {name!r}: duplicate description")
            descriptions.add(folded_description)

        tags = preset.get("tags")
        validated_tags: set[str] | None = None
        if not isinstance(tags, list) or len(tags) < 3:
            errors.add(f"{label} {name!r}: tags must be an array with at least three entries")
        else:
            folded_tags: set[str] = set()
            tags_valid = True
            for tag in tags:
                valid_tag = isinstance(tag, str) and bool(TAG_PATTERN.fullmatch(tag))
                errors.require(valid_tag, f"{label} {name!r}: invalid tag {tag!r}")
                tags_valid = tags_valid and valid_tag
                if isinstance(tag, str):
                    is_unique = tag not in folded_tags
                    errors.require(is_unique, f"{label} {name!r}: duplicate tag {tag!r}")
                    tags_valid = tags_valid and is_unique
                    folded_tags.add(tag)
            if tags_valid:
                validated_tags = folded_tags

        parameters = preset.get("parameters")
        if not isinstance(parameters, dict):
            errors.add(f"{label} {name!r}: parameters must be an object")
            continue

        actual_ids = set(parameters)
        expected_ids = set(PARAMETER_RANGES)
        errors.require(
            actual_ids == expected_ids,
            f"{label} {name!r}: parameter IDs differ; missing={sorted(expected_ids - actual_ids)}, "
            f"unknown={sorted(actual_ids - expected_ids)}",
        )
        if actual_ids != expected_ids:
            continue

        parameters_valid = True
        for parameter_id, (minimum, maximum) in PARAMETER_RANGES.items():
            value = parameters[parameter_id]
            if parameter_id == "legato":
                valid_bool = type(value) is bool
                errors.require(valid_bool, f"{label} {name!r}: legato must be a JSON boolean")
                parameters_valid = parameters_valid and valid_bool
                continue

            if not is_number(value):
                errors.add(f"{label} {name!r}: {parameter_id} must be numeric")
                parameters_valid = False
                continue

            try:
                numeric = float(value)
            except (OverflowError, ValueError):
                errors.add(f"{label} {name!r}: {parameter_id} must be a finite representable number")
                parameters_valid = False
                continue
            is_finite = math.isfinite(numeric)
            errors.require(is_finite, f"{label} {name!r}: {parameter_id} must be finite")
            if not is_finite:
                parameters_valid = False
                continue

            is_in_range = minimum <= numeric <= maximum
            errors.require(
                is_in_range,
                f"{label} {name!r}: {parameter_id}={value} outside [{minimum}, {maximum}]",
            )
            parameters_valid = parameters_valid and is_in_range

            if parameter_id in INTEGER_PARAMETERS:
                is_integral = numeric.is_integer()
                errors.require(is_integral, f"{label} {name!r}: {parameter_id} must be integral")
                parameters_valid = parameters_valid and is_integral
            if parameter_id in ENUM_VALUES:
                valid_enum = value in ENUM_VALUES[parameter_id]
                errors.require(valid_enum, f"{label} {name!r}: invalid {parameter_id} enum {value}")
                parameters_valid = parameters_valid and valid_enum

            interval = PARAMETER_INTERVALS[parameter_id]
            if interval > 0:
                snapped = snap_to_legal_value(numeric, minimum, maximum, interval)
                uses_legal_step = math.isclose(
                    numeric,
                    snapped,
                    rel_tol=0.0,
                    abs_tol=max(1.0e-8, interval * 1.0e-8),
                )
                errors.require(
                    uses_legal_step,
                    f"{label} {name!r}: {parameter_id}={value} does not use the JUCE interval {interval:g}",
                )
                parameters_valid = parameters_valid and uses_legal_step

        if not parameters_valid:
            continue

        validate_semantics(preset, parameters, index, errors)
        if validated_tags is not None:
            validate_tag_semantics(preset, parameters, validated_tags, index, errors)

        vector = tuple(parameters[parameter_id] for parameter_id in PARAMETER_RANGES)
        errors.require(vector not in full_vectors, f"{label} {name!r}: duplicate full parameter vector")
        full_vectors.add(vector)

        sonic_ids = [parameter_id for parameter_id in PARAMETER_RANGES if parameter_id not in NON_SOUND_PARAMETERS]
        sound_vector = tuple(parameters[parameter_id] for parameter_id in sonic_ids)
        errors.require(sound_vector not in sound_vectors, f"{label} {name!r}: differs only by non-sound parameters")
        sound_vectors.add(sound_vector)
        parameter_sets.append((name, parameters))

    errors.require(
        dict(categories) == EXPECTED_CATEGORY_COUNTS,
        f"Category counts differ: expected={EXPECTED_CATEGORY_COUNTS}, actual={dict(categories)}",
    )

    nearest_difference = 10_000
    nearest_pair = ("", "")
    for first_index, (first_name, first_parameters) in enumerate(parameter_sets):
        for second_name, second_parameters in parameter_sets[first_index + 1:]:
            difference = sound_difference_count(first_parameters, second_parameters)
            if difference < nearest_difference:
                nearest_difference = difference
                nearest_pair = (first_name, second_name)
            errors.require(
                difference >= 2,
                f"Presets {first_name!r} and {second_name!r} lack two meaningful sonic differences",
            )

    return (errors.messages, categories, nearest_difference if parameter_sets else 0)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("path", nargs="?", type=Path, default=DEFAULT_BANK_PATH)
    arguments = parser.parse_args()

    errors, categories, nearest_difference = validate_bank(arguments.path.resolve())
    if errors:
        print(f"Preset validation failed with {len(errors)} error(s):", file=sys.stderr)
        for message in errors:
            print(f"  - {message}", file=sys.stderr)
        return 1

    category_summary = ", ".join(
        f"{category}={categories[category]}" for category in EXPECTED_CATEGORY_COUNTS
    )
    print(f"Validated {EXPECTED_COUNT} curated presets: {category_summary}")
    print(f"Nearest preset pair still differs across {nearest_difference} meaningful sound dimensions.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
