#!/usr/bin/env python3
"""Generate the curated 808Glo Pro factory preset bank.

The source recipes intentionally live in code so the shipped JSON is deterministic,
reviewable, and can be regenerated without hand-editing production resources.
"""

from __future__ import annotations

import json
from collections import OrderedDict
from pathlib import Path
from typing import Any


SCHEMA_VERSION = 1
AUTHOR = "Diamond Loopz"
EXPECTED_COUNT = 128
ROOT = Path(__file__).resolve().parents[1]
OUTPUT_PATH = ROOT / "Resources" / "FactoryPresets.json"


# Inclusive ranges. Boolean and enum validation is repeated independently in
# validate_presets.py so generation and verification cannot silently agree on a typo.
PARAMETER_RANGES: "OrderedDict[str, tuple[float, float]]" = OrderedDict(
    [
        ("voiceMode", (0, 1)),
        ("legato", (0, 1)),
        ("triggerMode", (0, 1)),
        ("velocitySens", (0, 100)),
        ("tune", (-24, 24)),
        ("fine", (-100, 100)),
        ("bendRange", (1, 24)),
        ("glide", (0, 2000)),
        ("body", (0, 1)),
        ("harmonics", (0, 1)),
        ("harmonicBalance", (-1, 1)),
        ("pitchDrop", (0, 48)),
        ("pitchDecay", (5, 500)),
        ("pitchCurve", (0.25, 4)),
        ("attack", (0.05, 50)),
        ("hold", (0, 250)),
        ("decay", (50, 8000)),
        ("sustain", (0, 1)),
        ("release", (5, 3000)),
        ("ampCurve", (0.25, 4)),
        ("click", (0, 1)),
        ("clickTone", (500, 16000)),
        ("clickDecay", (0.1, 30)),
        ("punch", (0, 1)),
        ("tone", (45, 18000)),
        ("toneKeytrack", (0, 1)),
        ("drive", (0, 36)),
        ("driveMode", (0, 3)),
        ("compressor", (0, 1)),
        ("clipper", (0, 1)),
        ("output", (-24, 12)),
    ]
)


DEFAULT_PARAMETERS: dict[str, Any] = {
    "voiceMode": 0,
    "legato": False,
    "triggerMode": 0,
    "velocitySens": 58,
    "tune": 0,
    "fine": 0,
    "bendRange": 12,
    "glide": 0,
    "body": 0.12,
    "harmonics": 0.08,
    "harmonicBalance": 0.0,
    "pitchDrop": 18,
    "pitchDecay": 42,
    "pitchCurve": 1.2,
    "attack": 0.5,
    "hold": 0,
    "decay": 1200,
    "sustain": 0,
    "release": 90,
    "ampCurve": 1.7,
    "click": 0.12,
    "clickTone": 4200,
    "clickDecay": 3.5,
    "punch": 0.3,
    "tone": 6000,
    "toneKeytrack": 0.12,
    "drive": 2,
    "driveMode": 0,
    "compressor": 0.12,
    "clipper": 0.10,
    "output": -6,
}


CATEGORY_BASES: dict[str, dict[str, Any]] = {
    "Clean & Sub": {
        "velocitySens": 48, "body": 0.05, "harmonics": 0.03,
        "harmonicBalance": 0.20, "pitchDrop": 14, "pitchDecay": 55,
        "pitchCurve": 1.15, "attack": 0.8, "decay": 1900,
        "release": 220, "ampCurve": 1.85, "click": 0.04,
        "clickTone": 2600, "clickDecay": 2.8, "punch": 0.18,
        "tone": 1500, "toneKeytrack": 0.05, "drive": 0,
        "driveMode": 0, "compressor": 0.06, "clipper": 0.02,
        "output": -4.5,
    },
    "Trap": {
        "velocitySens": 62, "body": 0.22, "harmonics": 0.25,
        "harmonicBalance": -0.20, "pitchDrop": 27, "pitchDecay": 34,
        "pitchCurve": 0.75, "attack": 0.12, "decay": 1100,
        "release": 95, "ampCurve": 1.55, "click": 0.34,
        "clickTone": 6500, "clickDecay": 3.0, "punch": 0.62,
        "tone": 3600, "toneKeytrack": 0.16, "drive": 8,
        "driveMode": 2, "compressor": 0.30, "clipper": 0.45,
        "output": -7,
    },
    "Distorted": {
        "velocitySens": 55, "body": 0.55, "harmonics": 0.70,
        "harmonicBalance": -0.42, "pitchDrop": 23, "pitchDecay": 32,
        "pitchCurve": 0.80, "attack": 0.08, "decay": 1050,
        "release": 110, "ampCurve": 1.45, "click": 0.48,
        "clickTone": 7200, "clickDecay": 3.7, "punch": 0.68,
        "tone": 5200, "toneKeytrack": 0.22, "drive": 25,
        "driveMode": 3, "compressor": 0.48, "clipper": 0.78,
        "output": -10,
    },
    "Drill": {
        "legato": True, "triggerMode": 1, "velocitySens": 56,
        "glide": 185, "body": 0.18, "harmonics": 0.20,
        "harmonicBalance": -0.28, "pitchDrop": 22, "pitchDecay": 58,
        "pitchCurve": 1.05, "attack": 0.45, "decay": 1550,
        "sustain": 0.18, "release": 220, "ampCurve": 1.7,
        "click": 0.18, "clickTone": 4300, "clickDecay": 3.8,
        "punch": 0.43, "tone": 1600, "toneKeytrack": 0.22,
        "drive": 8, "driveMode": 2, "compressor": 0.30,
        "clipper": 0.35, "output": -6.5,
    },
    "Detroit": {
        "velocitySens": 72, "body": 0.29, "harmonics": 0.34,
        "harmonicBalance": -0.35, "pitchDrop": 27, "pitchDecay": 25,
        "pitchCurve": 0.62, "attack": 0.08, "decay": 390,
        "release": 55, "ampCurve": 1.32, "click": 0.58,
        "clickTone": 7600, "clickDecay": 2.3, "punch": 0.82,
        "tone": 3800, "toneKeytrack": 0.18, "drive": 10,
        "driveMode": 2, "compressor": 0.36, "clipper": 0.52,
        "output": -8,
    },
    "West Coast": {
        "legato": True, "velocitySens": 50, "glide": 120,
        "body": 0.20, "harmonics": 0.29, "harmonicBalance": 0.35,
        "pitchDrop": 13, "pitchDecay": 72, "pitchCurve": 1.30,
        "attack": 0.9, "decay": 1900, "sustain": 0.16,
        "release": 420, "ampCurve": 1.9, "click": 0.16,
        "clickTone": 3400, "clickDecay": 4.5, "punch": 0.34,
        "tone": 2500, "toneKeytrack": 0.26, "drive": 6,
        "driveMode": 1, "compressor": 0.22, "clipper": 0.20,
        "output": -6,
    },
    "Long Glide": {
        "legato": True, "triggerMode": 1, "velocitySens": 46,
        "bendRange": 24, "glide": 560, "body": 0.14,
        "harmonics": 0.14, "harmonicBalance": 0.18,
        "pitchDrop": 10, "pitchDecay": 90, "pitchCurve": 1.45,
        "attack": 2.0, "decay": 4300, "sustain": 0.76,
        "release": 1050, "ampCurve": 2.0, "click": 0.07,
        "clickTone": 2700, "clickDecay": 5.5, "punch": 0.20,
        "tone": 1200, "toneKeytrack": 0.28, "drive": 3,
        "driveMode": 1, "compressor": 0.16, "clipper": 0.12,
        "output": -4.5,
    },
    "Short Punch": {
        "velocitySens": 78, "body": 0.22, "harmonics": 0.24,
        "harmonicBalance": -0.28, "pitchDrop": 31,
        "pitchDecay": 18, "pitchCurve": 0.50, "attack": 0.05,
        "hold": 4, "decay": 250, "release": 30,
        "ampCurve": 1.15, "click": 0.70, "clickTone": 8400,
        "clickDecay": 1.8, "punch": 0.90, "tone": 5800,
        "toneKeytrack": 0.08, "drive": 8, "driveMode": 2,
        "compressor": 0.28, "clipper": 0.48, "output": -8,
    },
    "Experimental": {
        "velocitySens": 64, "bendRange": 24, "glide": 420,
        "body": 0.70, "harmonics": 0.82, "harmonicBalance": -0.55,
        "pitchDrop": 34, "pitchDecay": 180, "pitchCurve": 2.10,
        "attack": 4, "hold": 20, "decay": 2300,
        "sustain": 0.35, "release": 650, "ampCurve": 2.3,
        "click": 0.35, "clickTone": 10500, "clickDecay": 8,
        "punch": 0.52, "tone": 6200, "toneKeytrack": 0.55,
        "drive": 24, "driveMode": 3, "compressor": 0.44,
        "clipper": 0.70, "output": -11,
    },
    "Mix Ready": {
        "velocitySens": 58, "body": 0.15, "harmonics": 0.17,
        "harmonicBalance": 0.02, "pitchDrop": 18,
        "pitchDecay": 43, "pitchCurve": 0.95, "attack": 0.45,
        "decay": 1050, "release": 125, "ampCurve": 1.65,
        "click": 0.18, "clickTone": 4800, "clickDecay": 3.0,
        "punch": 0.42, "tone": 2400, "toneKeytrack": 0.14,
        "drive": 4, "driveMode": 1, "compressor": 0.34,
        "clipper": 0.24, "output": -6,
    },
}


CATEGORY_TAGS: dict[str, list[str]] = {
    "Clean & Sub": ["clean", "sub", "mono"],
    "Trap": ["trap", "modern", "hard"],
    "Distorted": ["distorted", "aggressive", "harmonic"],
    "Drill": ["drill", "glide", "mono-legato"],
    "Detroit": ["detroit", "short", "bounce"],
    "West Coast": ["west-coast", "warm", "glide"],
    "Long Glide": ["long", "glide", "sustained"],
    "Short Punch": ["short", "punch", "transient"],
    "Experimental": ["experimental", "fx", "wide-harmonics"],
    "Mix Ready": ["mix-ready", "controlled", "translation"],
}


def recipe(name: str, description: str, tags: list[str], **overrides: Any) -> dict[str, Any]:
    return {
        "name": name,
        "description": description,
        "tags": tags,
        "overrides": overrides,
    }


R = recipe


RECIPES: "OrderedDict[str, list[dict[str, Any]]]" = OrderedDict(
    [
        ("Clean & Sub", [
            R("Pure Current", "A pure sine-led foundation with a deep, even tail.", ["pure", "long"], voiceMode=0, legato=False, triggerMode=0, velocitySens=48, tune=0, fine=0, bendRange=12, glide=0, body=0.1393, harmonics=0.0565, harmonicBalance=0.1471, pitchDrop=10.18, pitchDecay=77.8717, pitchCurve=1.15, attack=0.1036, hold=0, decay=1243.5, sustain=0.0781, release=220, ampCurve=0.5801, click=0.117, clickTone=3073.4676, clickDecay=2.8, punch=0.1331, tone=2408.7577, toneKeytrack=0.05, drive=0.89, driveMode=0, compressor=0.0976, clipper=0.008, output=1.77),
            R("Deep Water", "A soft slow-blooming sub for sparse arrangements.", ["soft", "deep"], voiceMode=0, legato=False, triggerMode=0, velocitySens=48, tune=0, fine=0, bendRange=12, glide=0, body=0.0482, harmonics=0.0338, harmonicBalance=0.1855, pitchDrop=15.82, pitchDecay=69.1887, pitchCurve=1.15, attack=0.2557, hold=0, decay=1412.1613, sustain=0.1097, release=460, ampCurve=0.8982, click=0.1306, clickTone=3449.8388, clickDecay=2.8, punch=0.0203, tone=3172.7111, toneKeytrack=0.05, drive=1.55, driveMode=0, compressor=0.083, clipper=0.0302, output=1.05),
            R("Night Foundation", "A round clean 808 with a firmer front edge.", ["round", "firm"], voiceMode=0, legato=False, triggerMode=0, velocitySens=48, tune=0, fine=0, bendRange=12, glide=0, body=0.0408, harmonics=0.0306, harmonicBalance=0.2078, pitchDrop=8.75, pitchDecay=63.8758, pitchCurve=0.72, attack=0.3073, hold=0, decay=3096.8288, sustain=0.3486, release=220, ampCurve=0.6398, click=0.0735, clickTone=3364.5631, clickDecay=2.8, punch=0.0382, tone=3020.4497, toneKeytrack=0.05, drive=0.38, driveMode=0, compressor=0.0798, clipper=0.0062, output=-2.87),
            R("Silent Pressure", "An ultra-dark sustained sub that leaves the upper mix untouched.", ["dark", "sustained"], voiceMode=0, legato=True, triggerMode=1, velocitySens=48, tune=0, fine=0, bendRange=12, glide=120, body=0.1209, harmonics=0.0707, harmonicBalance=0.1629, pitchDrop=7.89, pitchDecay=68.5634, pitchCurve=1.15, attack=0.3345, hold=0, decay=1312.5, sustain=0.1332, release=760, ampCurve=0.5801, click=0.115, clickTone=3282.8136, clickDecay=2.8, punch=0.1384, tone=2833.6857, toneKeytrack=0.05, drive=0.82, driveMode=0, compressor=0.0334, clipper=0.02, output=2.25),
            R("Round Table", "A rounded triangle blend with restrained tape color.", ["rounded", "tape"], voiceMode=0, legato=False, triggerMode=0, velocitySens=48, tune=0, fine=0, bendRange=12, glide=0, body=0.0176, harmonics=0.0141, harmonicBalance=0.1328, pitchDrop=14.82, pitchDecay=60.5, pitchCurve=1.15, attack=0.1871, hold=0, decay=2000.3326, sustain=0.2276, release=220, ampCurve=0.9384, click=0.1098, clickTone=3357.4152, clickDecay=2.8, punch=0.1244, tone=2985.111, toneKeytrack=0.05, drive=1.44, driveMode=1, compressor=0.0633, clipper=0.0226, output=-2.12),
            R("Mono Gravity", "A weighty mono sub with a subtle playable slide.", ["weighty", "slide"], voiceMode=0, legato=True, triggerMode=1, velocitySens=48, tune=0, fine=0, bendRange=12, glide=82, body=0.1711, harmonics=0.0514, harmonicBalance=0.1896, pitchDrop=9.3, pitchDecay=72.2435, pitchCurve=0.62, attack=0.2684, hold=0, decay=1217.2, sustain=0.0547, release=360, ampCurve=0.8031, click=0.1318, clickTone=3036.7191, clickDecay=2.8, punch=0.0285, tone=2334.166, toneKeytrack=0.05, drive=1.17, driveMode=0, compressor=0.0716, clipper=0.026, output=2.12),
            R("Clean Sweep", "A clean high-impact drop that settles into an uncluttered sub.", ["impact", "clean"], voiceMode=0, legato=False, triggerMode=0, velocitySens=48, tune=0, fine=0, bendRange=12, glide=0, body=0.1621, harmonics=0.0134, harmonicBalance=0.2382, pitchDrop=31.96, pitchDecay=62.1765, pitchCurve=0.48, attack=0.2419, hold=0, decay=3010.4988, sustain=0.2909, release=75, ampCurve=0.5801, click=0.1049, clickTone=2869.6941, clickDecay=2.8, punch=0.089, tone=2900.7368, toneKeytrack=0.05, drive=0.78, driveMode=0, compressor=0.0524, clipper=0.0225, output=1.79),
            R("Ocean Floor", "A huge slow sub tail designed for open half-time patterns.", ["huge", "half-time"], voiceMode=0, legato=True, triggerMode=1, velocitySens=48, tune=0, fine=0, bendRange=12, glide=155, body=0.042, harmonics=0.0126, harmonicBalance=0.2809, pitchDrop=26.55, pitchDecay=92.3774, pitchCurve=1.8, attack=0.3525, hold=0, decay=1150, sustain=0.213, release=980, ampCurve=1.1822, click=0.0837, clickTone=3179.2453, clickDecay=2.8, punch=0.0849, tone=3622.5028, toneKeytrack=0.05, drive=1.5, driveMode=0, compressor=0.0601, clipper=0.0116, output=0.96),
            R("Fundamental One", "A reliable fundamental-first 808 for general production.", ["balanced", "utility"], voiceMode=0, legato=False, triggerMode=0, velocitySens=48, tune=0, fine=0, bendRange=12, glide=0, body=0.0253, harmonics=0.017, harmonicBalance=0.2757, pitchDrop=16.38, pitchDecay=82.3104, pitchCurve=1.15, attack=0.2968, hold=0, decay=1454.8, sustain=0.0666, release=210, ampCurve=0.4387, click=0.1205, clickTone=3433.1338, clickDecay=2.8, punch=0.1145, tone=3178.7234, toneKeytrack=0.05, drive=0.54, driveMode=0, compressor=0.0639, clipper=0.0228, output=2.19),
            R("Subtle Giant", "A broad clean body with just enough harmonic translation.", ["broad", "translation"], voiceMode=0, legato=False, triggerMode=0, velocitySens=48, tune=0, fine=0, bendRange=12, glide=0, body=0.0189, harmonics=0.0211, harmonicBalance=0.1291, pitchDrop=29.55, pitchDecay=75, pitchCurve=1.15, attack=0.2136, hold=0, decay=1800, sustain=0.0635, release=520, ampCurve=0.7259, click=0.1176, clickTone=3061.2181, clickDecay=2.8, punch=0.1219, tone=2607.6686, toneKeytrack=0.05, drive=1.32, driveMode=0, compressor=0.0735, clipper=0.0268, output=1.15),
            R("Glass Basement", "A polished sub with a light glassy second harmonic.", ["polished", "even-harmonic"], voiceMode=0, legato=False, triggerMode=0, velocitySens=48, tune=0, fine=0, bendRange=12, glide=0, body=0.0102, harmonics=0.0008, harmonicBalance=0.1348, pitchDrop=16.8, pitchDecay=77.9, pitchCurve=1.15, attack=0.3516, hold=0, decay=3100, sustain=0.3662, release=220, ampCurve=0.8541, click=0.1362, clickTone=3433.9623, clickDecay=2.8, punch=0.1258, tone=2425.5938, toneKeytrack=0.05, drive=1.54, driveMode=0, compressor=0.0965, clipper=0.02, output=-4.88),
            R("Low Meridian", "A stable tonal sub with gentle key tracking for melodic basslines.", ["tonal", "melodic"], voiceMode=1, legato=False, triggerMode=1, velocitySens=42, tune=0, fine=0, bendRange=12, glide=0, body=0.0627, harmonics=0.0401, harmonicBalance=0.1417, pitchDrop=10.95, pitchDecay=49.733, pitchCurve=0.4, attack=0.19, hold=0, decay=1475, sustain=0.0921, release=620, ampCurve=0.5304, click=0.1145, clickTone=3518.8679, clickDecay=2.8, punch=0.1296, tone=2723.2847, toneKeytrack=0.48, drive=0.32, driveMode=0, compressor=0.0798, clipper=0.0294, output=9.36),
        ]),
        ("Trap", [
            R("Redline Bounce", "A hard modern trap 808 with a fast redline knock.", ["fast", "knock"], voiceMode=0, legato=True, triggerMode=0, velocitySens=62, tune=0, fine=0, bendRange=12, glide=105, body=0.0914, harmonics=0.15, harmonicBalance=-0.8419, pitchDrop=11.8, pitchDecay=48.5, pitchCurve=0.58, attack=0.0774, hold=0, decay=956.0249, sustain=0.0315, release=95, ampCurve=0.5267, click=0.4172, clickTone=4877.9858, clickDecay=3, punch=0.3438, tone=7975.9069, toneKeytrack=0.16, drive=4.1, driveMode=2, compressor=0.1588, clipper=0.0807, output=-1.17),
            R("Funeral Trunk", "A dark long trap tail with controlled low-mid weight.", ["dark", "long"], voiceMode=0, legato=True, triggerMode=0, velocitySens=62, tune=0, fine=0, bendRange=12, glide=180, body=0.0873, harmonics=0.2899, harmonicBalance=-0.7373, pitchDrop=13.1, pitchDecay=51, pitchCurve=0.75, attack=0.1048, hold=0, decay=871.7, sustain=0.0629, release=320, ampCurve=0.7088, click=0.4584, clickTone=4745.283, clickDecay=3, punch=0.1207, tone=5904.2553, toneKeytrack=0.16, drive=11.79, driveMode=1, compressor=0.3459, clipper=0.0891, output=0.05),
            R("Crown Pressure", "A clipped crown-heavy hit for aggressive sparse drums.", ["clipped", "aggressive"], voiceMode=0, legato=False, triggerMode=0, velocitySens=62, tune=0, fine=0, bendRange=12, glide=0, body=0.1033, harmonics=0.1627, harmonicBalance=-0.8146, pitchDrop=12.41, pitchDecay=57.2233, pitchCurve=0.43, attack=0.1937, hold=0, decay=1543, sustain=0.3581, release=65, ampCurve=0.7354, click=0.4078, clickTone=4867.9245, clickDecay=3, punch=0.1371, tone=6106.383, toneKeytrack=0.16, drive=12.29, driveMode=3, compressor=0.2023, clipper=0.1181, output=-3.64),
            R("Dark Alley Bend", "A low-passed trap slide with a patient pitch bend.", ["dark", "slide"], voiceMode=0, legato=True, triggerMode=1, velocitySens=62, tune=0, fine=0, bendRange=12, glide=265, body=0.2697, harmonics=0.24, harmonicBalance=-0.5448, pitchDrop=15.8, pitchDecay=55.9, pitchCurve=1.55, attack=0.1597, hold=0, decay=703.9, sustain=0.1259, release=440, ampCurve=0.6022, click=0.4405, clickTone=4990.566, clickDecay=3, punch=0.2608, tone=8325.8426, toneKeytrack=0.16, drive=11.9, driveMode=2, compressor=0.2241, clipper=0.1368, output=-1.84),
            R("Chrome Rattle", "A bright metallic trap body that cuts through small speakers.", ["bright", "metallic"], voiceMode=0, legato=False, triggerMode=0, velocitySens=62, tune=0, fine=0, bendRange=12, glide=0, body=0.3291, harmonics=0.27, harmonicBalance=-0.6368, pitchDrop=17.2, pitchDecay=71.375, pitchCurve=0.75, attack=0.2854, hold=0, decay=668.3, sustain=0.1574, release=95, ampCurve=0.4876, click=0.4523, clickTone=6214.5877, clickDecay=3, punch=0.1447, tone=7753.2524, toneKeytrack=0.24, drive=2, driveMode=3, compressor=0.2458, clipper=0.1555, output=-2.24),
            R("Midnight Flex", "A deep smooth trap 808 made for long held notes.", ["smooth", "held"], voiceMode=0, legato=True, triggerMode=1, velocitySens=62, tune=0, fine=0, bendRange=12, glide=320, body=0.1381, harmonics=0.2762, harmonicBalance=-0.5269, pitchDrop=18.5, pitchDecay=69.8925, pitchCurve=1.7, attack=0.3977, hold=0, decay=816.7, sustain=0.1888, release=560, ampCurve=0.4511, click=0.3357, clickTone=5157.8585, clickDecay=3, punch=0.2052, tone=6011.638, toneKeytrack=0.16, drive=2.48, driveMode=1, compressor=0.2676, clipper=0.1742, output=-1.42),
            R("Heavy Motion", "A balanced trap workhorse with hard punch and usable glide.", ["workhorse", "punchy"], voiceMode=0, legato=True, triggerMode=0, velocitySens=62, tune=0, fine=0, bendRange=12, glide=150, body=0.0405, harmonics=0.3493, harmonicBalance=-0.7148, pitchDrop=16.73, pitchDecay=65.18, pitchCurve=0.52, attack=0.3299, hold=0, decay=908.7499, sustain=0.0496, release=95, ampCurve=0.4297, click=0.4722, clickTone=4810.4378, clickDecay=3, punch=0.228, tone=7864.5797, toneKeytrack=0.16, drive=6.54, driveMode=2, compressor=0.3859, clipper=0.1377, output=-2.84),
            R("Black Diamond", "A polished asymmetrical 808 with expensive low-mid grit.", ["polished", "asymmetrical"], voiceMode=0, legato=True, triggerMode=1, velocitySens=62, tune=0, fine=0, bendRange=12, glide=215, body=0.0999, harmonics=0.36, harmonicBalance=-0.8419, pitchDrop=28.42, pitchDecay=54.325, pitchCurve=0.75, attack=0.2258, hold=0, decay=620, sustain=0.1274, release=245, ampCurve=0.9044, click=0.4169, clickTone=5519.1478, clickDecay=3, punch=0.2507, tone=6559.3697, toneKeytrack=0.16, drive=9.8, driveMode=2, compressor=0.3274, clipper=0.3235, output=-5.49),
            R("Glo Season", "A sharp short-drop trap hit with an oversized first transient.", ["sharp", "transient"], voiceMode=0, legato=False, triggerMode=0, velocitySens=62, tune=0, fine=0, bendRange=12, glide=0, body=0.1593, harmonics=0.39, harmonicBalance=-0.6579, pitchDrop=14.97, pitchDecay=61.95, pitchCurve=0.32, attack=0.2968, hold=0, decay=1291.3, sustain=0.4178, release=45, ampCurve=0.3252, click=0.4168, clickTone=6134.9659, clickDecay=1.5, punch=0.2734, tone=7622.0263, toneKeytrack=0.16, drive=12.4, driveMode=2, compressor=0.3329, clipper=0.2304, output=-8.04),
            R("Trunk Ritual", "A slow dirty trunk rumbler with a wide tonal footprint.", ["rumble", "dirty"], voiceMode=0, legato=True, triggerMode=1, velocitySens=62, tune=0, fine=0, bendRange=12, glide=285, body=0.2188, harmonics=0.42, harmonicBalance=-0.584, pitchDrop=24, pitchDecay=70.8, pitchCurve=2, attack=0.3242, hold=0, decay=665.6, sustain=0.1422, release=690, ampCurve=0.4876, click=0.4297, clickTone=5934.7152, clickDecay=3, punch=0.2961, tone=7521.2766, toneKeytrack=0.16, drive=2.5, driveMode=1, compressor=0.3364, clipper=0.0891, output=-4.46),
            R("Onyx Knock", "A dry centered trap knock with a disciplined tail.", ["dry", "centered"], voiceMode=0, legato=False, triggerMode=0, velocitySens=62, tune=0, fine=0, bendRange=12, glide=0, body=0.2782, harmonics=0.1227, harmonicBalance=-0.5091, pitchDrop=25.22, pitchDecay=80.7475, pitchCurve=0.75, attack=0.414, hold=0, decay=1084.9, sustain=0.2468, release=70, ampCurve=0.7088, click=0.362, clickTone=4925.837, clickDecay=2, punch=0.2852, tone=6122.965, toneKeytrack=0.16, drive=5.1, driveMode=2, compressor=0.3765, clipper=0.2678, output=-5.79),
            R("Pressure Point", "A compressed mid-forward trap 808 for dense arrangements.", ["compressed", "mid-forward"], voiceMode=0, legato=False, triggerMode=0, velocitySens=62, tune=0, fine=0, bendRange=12, glide=0, body=0.3376, harmonics=0.3362, harmonicBalance=-0.5181, pitchDrop=26.7, pitchDecay=75.8, pitchCurve=0.75, attack=0.379, hold=0, decay=655.5, sustain=0.3777, release=95, ampCurve=0.4007, click=0.4557, clickTone=5920.2482, clickDecay=3, punch=0.3415, tone=7268.1462, toneKeytrack=0.16, drive=7.7, driveMode=2, compressor=0.3863, clipper=0.1269, output=-3.5),
            R("Dead Center", "A mono-solid trap fundamental with no wasted upper fizz.", ["mono", "focused"], voiceMode=0, legato=False, triggerMode=0, velocitySens=62, tune=0, fine=0, bendRange=12, glide=0, body=0.1855, harmonics=0.3493, harmonicBalance=-0.5659, pitchDrop=28, pitchDecay=78.3, pitchCurve=0.75, attack=0.4065, hold=0, decay=718.4, sustain=0.4092, release=95, ampCurve=0.8721, click=0.3679, clickTone=6094.3396, clickDecay=3, punch=0.2464, tone=8127.6596, toneKeytrack=0.16, drive=3.97, driveMode=1, compressor=0.1387, clipper=0.3052, output=-3.84),
            R("Afterdark Weight", "A sustained after-hours trap bass with smooth legato movement.", ["sustained", "legato"], voiceMode=0, legato=True, triggerMode=1, velocitySens=62, tune=0, fine=0, bendRange=12, glide=390, body=0.049, harmonics=0.2127, harmonicBalance=-0.4522, pitchDrop=29.4, pitchDecay=59.0375, pitchCurve=1.9, attack=0.27, hold=0, decay=1123.4, sustain=0.2615, release=850, ampCurve=0.8541, click=0.446, clickTone=6216.9811, clickDecay=3, punch=0.1325, tone=8329.7872, toneKeytrack=0.16, drive=12.9, driveMode=1, compressor=0.161, clipper=0.16, output=-10.88),
        ]),
        ("Distorted", [
            R("Furnace Teeth", "A hard-clipped furnace 808 with retained fundamental weight.", ["hard-clip", "dense"], voiceMode=0, legato=False, triggerMode=0, velocitySens=55, tune=0, fine=0, bendRange=12, glide=0, body=0.3499, harmonics=0.5, harmonicBalance=-0.9535, pitchDrop=5, pitchDecay=49.4, pitchCurve=0.8, attack=0.0774, hold=0, decay=1263.1, sustain=0.0254, release=110, ampCurve=0.5067, click=0.2927, clickTone=4641.5094, clickDecay=3.7, punch=0.0351, tone=5191.4894, toneKeytrack=0.22, drive=20.4, driveMode=3, compressor=0.2991, clipper=0.3574, output=-6.67),
            R("Burn Notice", "A warm asymmetrical burn with a longer musical tail.", ["warm", "asymmetrical"], voiceMode=0, legato=True, triggerMode=0, velocitySens=55, tune=0, fine=0, bendRange=12, glide=105, body=0.5927, harmonics=0.8274, harmonicBalance=-0.907, pitchDrop=6.8, pitchDecay=52.8, pitchCurve=0.8, attack=0.1048, hold=0, decay=838.4, sustain=0.0508, release=190, ampCurve=0.7133, click=0.3054, clickTone=4783.0189, clickDecay=3.7, punch=0.0592, tone=5382.9787, toneKeytrack=0.22, drive=24.3, driveMode=2, compressor=0.4085, clipper=0.3588, output=-6.36),
            R("Crushed Chrome", "A bright crushed 808 for maximal high-frequency presence.", ["bright", "crushed"], voiceMode=0, legato=False, triggerMode=0, velocitySens=55, tune=0, fine=0, bendRange=12, glide=0, body=0.5666, harmonics=0.6, harmonicBalance=-0.8605, pitchDrop=8.6, pitchDecay=56.2, pitchCurve=0.38, attack=0.1323, hold=0, decay=768.8689, sustain=0.0763, release=55, ampCurve=0.5429, click=0.3181, clickTone=6244.99, clickDecay=1.2, punch=0.0834, tone=5574.4681, toneKeytrack=0.22, drive=28.2, driveMode=3, compressor=0.3192, clipper=0.3602, output=-5.19),
            R("Rabid Circuit", "A folding circuit-bass with animated odd-harmonic bite.", ["folded", "odd-harmonic"], voiceMode=0, legato=True, triggerMode=1, velocitySens=55, tune=0, fine=0, bendRange=12, glide=245, body=0.6749, harmonics=0.65, harmonicBalance=-0.814, pitchDrop=10.4, pitchDecay=59.7, pitchCurve=1.8, attack=0.1597, hold=0, decay=626.1, sustain=0.1017, release=290, ampCurve=0.5756, click=0.3308, clickTone=5066.0377, clickDecay=3.7, punch=0.1075, tone=5765.9574, toneKeytrack=0.22, drive=32.1, driveMode=3, compressor=0.3293, clipper=0.3616, output=-4.43),
            R("Rusted Crown", "A tube-scorched 808 with a thick controlled midrange.", ["tube", "thick"], voiceMode=0, legato=True, triggerMode=0, velocitySens=55, tune=0, fine=0, bendRange=12, glide=150, body=0.7833, harmonics=0.6884, harmonicBalance=-0.7674, pitchDrop=12.2, pitchDecay=63.1, pitchCurve=0.8, attack=0.1871, hold=0, decay=1475.4, sustain=0.2995, release=360, ampCurve=0.7822, click=0.3435, clickTone=5207.5472, clickDecay=3.7, punch=0.1316, tone=5957.4468, toneKeytrack=0.22, drive=17.2, driveMode=1, compressor=0.341, clipper=0.3687, output=-8.88),
            R("Asphalt Grinder", "A slow grinding 808 with resonant low-mid distortion.", ["resonant", "slow"], voiceMode=0, legato=True, triggerMode=1, velocitySens=55, tune=0, fine=0, bendRange=12, glide=305, body=0.8916, harmonics=0.75, harmonicBalance=-0.7209, pitchDrop=14.1, pitchDecay=66.5, pitchCurve=2.2, attack=0.2145, hold=0, decay=1050.7, sustain=0.1525, release=620, ampCurve=0.4378, click=0.3562, clickTone=5349.0566, clickDecay=3.7, punch=0.1557, tone=6148.9362, toneKeytrack=0.22, drive=21.1, driveMode=3, compressor=0.3495, clipper=0.3645, output=-7.8),
            R("Voltage Scar", "A short voltage spike with a clean sub beneath the damage.", ["short", "layered"], voiceMode=0, legato=False, triggerMode=0, velocitySens=55, tune=0, fine=0, bendRange=12, glide=0, body=0.257, harmonics=0.8, harmonicBalance=-0.6744, pitchDrop=15.9, pitchDecay=69.9, pitchCurve=0.44, attack=0.2419, hold=0, decay=1171.5475, sustain=0.1779, release=62, ampCurve=0.697, click=0.3689, clickTone=5042.3795, clickDecay=3.7, punch=0.1799, tone=6340.4255, toneKeytrack=0.22, drive=25.1, driveMode=3, compressor=0.3595, clipper=0.3659, output=-7.34),
            R("Broken Amp", "A blown-amplifier texture with an uneven aggressive edge.", ["blown-amp", "rough"], voiceMode=0, legato=True, triggerMode=1, velocitySens=55, tune=0, fine=0, bendRange=12, glide=182, body=0.3653, harmonics=0.85, harmonicBalance=-0.6279, pitchDrop=17.7, pitchDecay=73.3, pitchCurve=0.8, attack=0.2694, hold=0, decay=520, sustain=0.2034, release=230, ampCurve=0.8511, click=0.3816, clickTone=5632.0755, clickDecay=3.7, punch=0.204, tone=6531.9149, toneKeytrack=0.22, drive=29, driveMode=2, compressor=0.3696, clipper=0.3673, output=-8.49),
            R("Red Static", "An extreme short static hit for rage and industrial patterns.", ["extreme", "rage"], voiceMode=0, legato=False, triggerMode=0, velocitySens=55, tune=0, fine=0, bendRange=12, glide=0, body=0.4737, harmonics=0.9, harmonicBalance=-0.5814, pitchDrop=19.5, pitchDecay=76.7, pitchCurve=0.27, attack=0.2968, hold=0, decay=1369.3, sustain=0.2288, release=35, ampCurve=0.323, click=0.3943, clickTone=5773.5849, clickDecay=0.8, punch=0.2281, tone=6723.4043, toneKeytrack=0.22, drive=32.9, driveMode=3, compressor=0.3797, clipper=0.3687, output=-1.31),
            R("Iron Lung", "A dark breathing distortion with a very long gated tail.", ["dark", "gated"], voiceMode=0, legato=True, triggerMode=1, velocitySens=55, tune=0, fine=0, bendRange=12, glide=405, body=0.5821, harmonics=0.95, harmonicBalance=-0.4264, pitchDrop=10.87, pitchDecay=80.1, pitchCurve=2.4, attack=0.3914, hold=0, decay=944.6, sustain=0.0359, release=920, ampCurve=0.5296, click=0.3217, clickTone=5915.0943, clickDecay=3.7, punch=0.1585, tone=5605.8871, toneKeytrack=0.22, drive=18, driveMode=1, compressor=0.3898, clipper=0.371, output=-9.16),
            R("Concrete Shred", "A midrange shredder tuned for audible laptop-speaker grit.", ["midrange", "speaker-ready"], voiceMode=0, legato=False, triggerMode=0, velocitySens=55, tune=0, fine=0, bendRange=12, glide=0, body=0.321, harmonics=0.4545, harmonicBalance=-0.4884, pitchDrop=23.1, pitchDecay=83.6, pitchCurve=0.8, attack=0.3516, hold=0, decay=1793.9, sustain=0.2796, release=110, ampCurve=0.6537, click=0.4197, clickTone=6056.6038, clickDecay=3.7, punch=0.2763, tone=7106.383, toneKeytrack=0.22, drive=30.74, driveMode=3, compressor=0.3998, clipper=0.3715, output=-4.22),
            R("Circuit Bruise", "A bruised soft-clip tone with heavy even-harmonic bloom.", ["soft-clip", "even-harmonic"], voiceMode=0, legato=False, triggerMode=0, velocitySens=55, tune=0, fine=0, bendRange=12, glide=0, body=0.841, harmonics=0.5211, harmonicBalance=-0.4419, pitchDrop=24.9, pitchDecay=87, pitchCurve=0.8, attack=0.379, hold=0, decay=732.2, sustain=0.3051, release=330, ampCurve=0.3919, click=0.4324, clickTone=6198.1132, clickDecay=3.7, punch=0.3005, tone=7297.8723, toneKeytrack=0.22, drive=25.8, driveMode=2, compressor=0.3343, clipper=0.3729, output=-7.21),
            R("Speaker Torch", "A fiercely bright clipped 808 designed for controlled destruction.", ["bright", "clipped"], voiceMode=0, legato=False, triggerMode=0, velocitySens=55, tune=0, fine=0, bendRange=12, glide=0, body=0.9071, harmonics=0.5545, harmonicBalance=-0.3953, pitchDrop=26.7, pitchDecay=90.4, pitchCurve=0.8, attack=0.4065, hold=0, decay=1581.5, sustain=0.3305, release=110, ampCurve=0.5985, click=0.4451, clickTone=6339.6226, clickDecay=1.1, punch=0.3246, tone=7489.3617, toneKeytrack=0.35, drive=29.7, driveMode=3, compressor=0.2898, clipper=0.3744, output=-3.83),
        ]),
        ("Drill", [
            R("Cold Step", "A dark controlled drill 808 with a precise medium slide.", ["dark", "controlled"], voiceMode=0, legato=True, triggerMode=1, velocitySens=56, tune=0, fine=0, bendRange=12, glide=180, body=0.0679, harmonics=0.0909, harmonicBalance=-0.9326, pitchDrop=6.9, pitchDecay=48.8, pitchCurve=1.05, attack=0.0774, hold=0, decay=937, sustain=0.0351, release=190, ampCurve=0.5667, click=0.2316, clickTone=5197.7908, clickDecay=3.8, punch=0.0238, tone=5901.1745, toneKeytrack=0.22, drive=3.4, driveMode=2, compressor=0.0974, clipper=0.0595, output=0.33),
            R("Sliding Shadow", "A deep long drill slide that moves without losing pitch center.", ["deep", "long-slide"], voiceMode=0, legato=True, triggerMode=1, velocitySens=56, tune=0, fine=0, bendRange=12, glide=425, body=0.1359, harmonics=0.1218, harmonicBalance=-0.9151, pitchDrop=31.96, pitchDecay=57.1052, pitchCurve=1.6, attack=0.2355, hold=0, decay=1837.2, sustain=0.3563, release=380, ampCurve=0.8912, click=0.3315, clickTone=5211.1448, clickDecay=3.8, punch=0.0477, tone=5922.5078, toneKeytrack=0.22, drive=2.65, driveMode=1, compressor=0.1243, clipper=0.186, output=-3.45),
            R("Frozen Block", "A hard icy drill hit with a fast defined glide.", ["hard", "icy"], voiceMode=0, legato=True, triggerMode=1, velocitySens=56, tune=0, fine=0, bendRange=12, glide=138, body=0.0832, harmonics=0.1775, harmonicBalance=-0.8977, pitchDrop=21.06, pitchDecay=54.5, pitchCurve=0.66, attack=0.1848, hold=0, decay=2082.8, sustain=0.3335, release=95, ampCurve=0.4156, click=0.31, clickTone=4339.6226, clickDecay=3.8, punch=0.2521, tone=5042.5532, toneKeytrack=0.22, drive=13.45, driveMode=3, compressor=0.1512, clipper=0.173, output=-4.95),
            R("Concrete Freeze", "An ultra-low sustained drill foundation for sparse piano loops.", ["sustained", "ultra-low"], voiceMode=0, legato=True, triggerMode=1, velocitySens=56, tune=0, fine=0, bendRange=12, glide=365, body=0.2717, harmonics=0.1836, harmonicBalance=-0.8802, pitchDrop=19.59, pitchDecay=92.3774, pitchCurve=1.9, attack=0.3763, hold=0, decay=1026.2377, sustain=0.1532, release=520, ampCurve=0.7801, click=0.2581, clickTone=5306.5304, clickDecay=3.8, punch=0.0953, tone=6160.2742, toneKeytrack=0.22, drive=13.56, driveMode=1, compressor=0.3316, clipper=0.0701, output=-2.83),
            R("Dark March", "A gritty marching drill bass with an assertive upper edge.", ["gritty", "mid-forward"], voiceMode=0, legato=True, triggerMode=1, velocitySens=56, tune=0, fine=0, bendRange=12, glide=255, body=0.3396, harmonics=0.298, harmonicBalance=-0.8413, pitchDrop=16.78, pitchDecay=83.5548, pitchCurve=1.05, attack=0.1578, hold=0, decay=1865.7, sustain=0.0655, release=155, ampCurve=0.8689, click=0.2657, clickTone=5367.7778, clickDecay=3.8, punch=0.0365, tone=6258.1181, toneKeytrack=0.22, drive=0.7, driveMode=2, compressor=0.1015, clipper=0.0855, output=-3.49),
            R("Grit Slide", "A saturated drill slide with audible movement on small speakers.", ["saturated", "speaker-ready"], voiceMode=0, legato=True, triggerMode=1, velocitySens=56, tune=0, fine=0, bendRange=12, glide=325, body=0.2021, harmonics=0.2578, harmonicBalance=-0.8453, pitchDrop=15.33, pitchDecay=54.9316, pitchCurve=1.05, attack=0.2144, hold=0, decay=1482.971, sustain=0.1725, release=270, ampCurve=0.7431, click=0.3226, clickTone=5162.1469, clickDecay=3.8, punch=0.143, tone=5585.1064, toneKeytrack=0.22, drive=12.7, driveMode=3, compressor=0.3752, clipper=0.0812, output=-0.43),
            R("Tension Wire", "A resonant slow-bending 808 that builds suspense between notes.", ["resonant", "slow-bend"], voiceMode=0, legato=True, triggerMode=1, velocitySens=56, tune=0, fine=0, bendRange=12, glide=505, body=0.0097, harmonics=0.2452, harmonicBalance=-0.8643, pitchDrop=30.99, pitchDecay=59.1217, pitchCurve=2.3, attack=0.2419, hold=0, decay=2300, sustain=0.3376, release=720, ampCurve=0.7178, click=0.3014, clickTone=4792.4528, clickDecay=3.8, punch=0.0496, tone=6059.4891, toneKeytrack=0.22, drive=7.5, driveMode=2, compressor=0.3605, clipper=0.0918, output=-4.9),
            R("Alley Phantom", "A nearly pure phantom sub with an extra-long legato tail.", ["pure", "phantom"], voiceMode=0, legato=True, triggerMode=1, velocitySens=56, tune=0, fine=0, bendRange=12, glide=610, body=0.0776, harmonics=0.3073, harmonicBalance=-0.9233, pitchDrop=15.6, pitchDecay=68.6, pitchCurve=1.05, attack=0.2694, hold=0, decay=1891.4855, sustain=0.1725, release=840, ampCurve=0.9121, click=0.313, clickTone=4924.2351, clickDecay=3.8, punch=0.1906, tone=5549.5507, toneKeytrack=0.22, drive=3.13, driveMode=1, compressor=0.3463, clipper=0.0812, output=-4.6),
            R("Black Puffer", "A compact hard drill knock with rapid pitch settling.", ["compact", "knock"], voiceMode=0, legato=True, triggerMode=1, velocitySens=56, tune=0, fine=0, bendRange=12, glide=92, body=0.2842, harmonics=0.075, harmonicBalance=-0.793, pitchDrop=18.58, pitchDecay=89.2104, pitchCurve=0.42, attack=0.2968, hold=0, decay=1757.1, sustain=0.1, release=68, ampCurve=0.3652, click=0.3246, clickTone=5018.8679, clickDecay=3.8, punch=0.2145, tone=6127.6596, toneKeytrack=0.22, drive=8.42, driveMode=2, compressor=0.3125, clipper=0.1029, output=-5.92),
            R("Winter Pressure", "A balanced winter-dark slide suitable for full drill arrangements.", ["balanced", "dark"], voiceMode=0, legato=True, triggerMode=1, velocitySens=56, tune=0, fine=0, bendRange=12, glide=282, body=0.2135, harmonics=0.235, harmonicBalance=-0.9254, pitchDrop=21.58, pitchDecay=98.5991, pitchCurve=1.75, attack=0.406, hold=0, decay=2222, sustain=0.3511, release=330, ampCurve=0.4697, click=0.2707, clickTone=5132.0755, clickDecay=3.8, punch=0.2521, tone=6308.5106, toneKeytrack=0.22, drive=6.32, driveMode=2, compressor=0.3285, clipper=0.0678, output=-0.9),
            R("Steel Slide", "A bright steel-edged drill glide for fast octave movement.", ["bright", "octave-slide"], voiceMode=0, legato=True, triggerMode=1, velocitySens=56, tune=0, fine=0, bendRange=12, glide=205, body=0.2609, harmonics=0.0628, harmonicBalance=-0.7581, pitchDrop=26.55, pitchDecay=77.1, pitchCurve=1.05, attack=0.3516, hold=0, decay=1682.2, sustain=0.2527, release=180, ampCurve=0.4488, click=0.3478, clickTone=4556.3986, clickDecay=3.8, punch=0.2621, tone=6489.3617, toneKeytrack=0.38, drive=5.83, driveMode=2, compressor=0.3662, clipper=0.1269, output=-7.67),
            R("North Wind", "A smooth cold glide with softened transients and a long release.", ["smooth", "soft-transient"], voiceMode=0, legato=True, triggerMode=1, velocitySens=56, tune=0, fine=0, bendRange=12, glide=470, body=0.3493, harmonics=0.2578, harmonicBalance=-0.7407, pitchDrop=21.36, pitchDecay=97.9208, pitchCurve=2, attack=0.379, hold=0, decay=1105.6, sustain=0.1532, release=950, ampCurve=0.4407, click=0.3595, clickTone=5358.4906, clickDecay=3.8, punch=0.286, tone=6670.2128, toneKeytrack=0.22, drive=8.2, driveMode=2, compressor=0.3752, clipper=0.1029, output=-8.4),
            R("Ghost Route", "A nimble ghost-note drill 808 with a short controllable slide.", ["nimble", "ghost-note"], voiceMode=0, legato=True, triggerMode=1, velocitySens=56, tune=0, fine=0, bendRange=12, glide=115, body=0.0465, harmonics=0.1526, harmonicBalance=-0.7233, pitchDrop=21.9, pitchDecay=82.8, pitchCurve=1.05, attack=0.4065, hold=0, decay=1974.3, sustain=0.4564, release=110, ampCurve=0.6674, click=0.3711, clickTone=5471.6981, clickDecay=3.8, punch=0.3098, tone=6851.0638, toneKeytrack=0.22, drive=12.23, driveMode=2, compressor=0.0726, clipper=0.2999, output=-11.77),
        ]),
        ("Detroit", [
            R("Motor Bounce", "A short motor-city bounce with a crisp front edge.", ["crisp", "classic"], voiceMode=0, legato=False, triggerMode=0, velocitySens=72, tune=0, fine=0, bendRange=12, glide=0, body=0.2662, harmonics=0.466, harmonicBalance=-0.9117, pitchDrop=7, pitchDecay=47, pitchCurve=0.62, attack=0.0774, hold=0, decay=616.4, sustain=0.0048, release=55, ampCurve=0.44, click=0.3635, clickTone=5113.2075, clickDecay=2.3, punch=0.3268, tone=6702.1277, toneKeytrack=0.18, drive=7.09, driveMode=2, compressor=0.3425, clipper=0.4835, output=-6.23),
            R("Offbeat Money", "A slightly rounder short 808 built for syncopated pockets.", ["syncopated", "round"], voiceMode=0, legato=False, triggerMode=0, velocitySens=72, tune=0, fine=0, bendRange=12, glide=0, body=0.1809, harmonics=0.1397, harmonicBalance=-0.8709, pitchDrop=8.8, pitchDecay=48.1, pitchCurve=0.62, attack=0.1048, hold=0, decay=412.7, sustain=0.0097, release=72, ampCurve=0.58, click=0.377, clickTone=5226.4151, clickDecay=2.3, punch=0.3437, tone=6904.2553, toneKeytrack=0.18, drive=8.4, driveMode=2, compressor=0.3489, clipper=0.4771, output=-6.15),
            R("Buffed Up", "An extremely tight bright knock with hard clipping.", ["tight", "bright"], voiceMode=0, legato=False, triggerMode=0, velocitySens=72, tune=0, fine=0, bendRange=12, glide=0, body=0.2403, harmonics=0.1941, harmonicBalance=-0.8301, pitchDrop=10.6, pitchDecay=49.1, pitchCurve=0.34, attack=0.1323, hold=0, decay=279.2874, sustain=0.0145, release=34, ampCurve=0.3467, click=0.3905, clickTone=6107.4089, clickDecay=1, punch=0.3605, tone=7952.361, toneKeytrack=0.18, drive=11.6, driveMode=3, compressor=0.3554, clipper=0.4706, output=-6.84),
            R("Woodward Knock", "A woody centered knock with extra fundamental beneath it.", ["woody", "centered"], voiceMode=0, legato=False, triggerMode=0, velocitySens=72, tune=0, fine=0, bendRange=12, glide=0, body=0.2997, harmonics=0.2485, harmonicBalance=-0.7892, pitchDrop=12.4, pitchDecay=50.1, pitchCurve=0.62, attack=0.1597, hold=0, decay=310.9, sustain=0.0194, release=105, ampCurve=0.4867, click=0.4041, clickTone=5452.8302, clickDecay=3.5, punch=0.3773, tone=7308.5106, toneKeytrack=0.18, drive=14.7, driveMode=1, compressor=0.3618, clipper=0.4641, output=-4.26),
            R("Motor City Glide", "A rare sliding Detroit 808 that stays short and rhythmic.", ["slide", "rhythmic"], voiceMode=0, legato=True, triggerMode=1, velocitySens=72, tune=0, fine=0, bendRange=12, glide=92, body=0.3591, harmonics=0.3028, harmonicBalance=-0.7484, pitchDrop=14.2, pitchDecay=51.2, pitchCurve=0.62, attack=0.1871, hold=0, decay=606.1, sustain=0.0242, release=145, ampCurve=0.6211, click=0.4176, clickTone=5566.0377, clickDecay=2.3, punch=0.3941, tone=7510.6383, toneKeytrack=0.18, drive=3.87, driveMode=1, compressor=0.3683, clipper=0.4576, output=-5.02),
            R("Punchline Pocket", "A one-shot punctuation hit with maximum initial punch.", ["one-shot", "maximum-punch"], voiceMode=0, legato=False, triggerMode=0, velocitySens=72, tune=0, fine=0, bendRange=12, glide=0, body=0.4186, harmonics=0.3572, harmonicBalance=-0.7076, pitchDrop=16.1, pitchDecay=52.2, pitchCurve=0.28, attack=0.2145, hold=0, decay=514.5, sustain=0.0291, release=20, ampCurve=0.3933, click=0.4311, clickTone=5679.2453, clickDecay=0.7, punch=0.411, tone=7712.766, toneKeytrack=0.18, drive=5.9, driveMode=2, compressor=0.3748, clipper=0.4512, output=-6.51),
            R("Fast Talk", "A rapid clean-cut 808 for dense talk-over-the-beat patterns.", ["rapid", "dense-pattern"], voiceMode=0, legato=False, triggerMode=0, velocitySens=72, tune=0, fine=0, bendRange=12, glide=0, body=0.0705, harmonics=0.4115, harmonicBalance=-0.6668, pitchDrop=17.9, pitchDecay=53.2, pitchCurve=0.62, attack=0.2419, hold=0, decay=445.5, sustain=0.0339, release=42, ampCurve=0.5003, click=0.4446, clickTone=5168.3386, clickDecay=1.3, punch=0.4278, tone=7914.8936, toneKeytrack=0.18, drive=9.1, driveMode=2, compressor=0.3812, clipper=0.4447, output=-7.19),
            R("Coney Bass", "A dark rounded Detroit bass with a little more tail.", ["dark", "rounded"], voiceMode=0, legato=False, triggerMode=0, velocitySens=72, tune=0, fine=0, bendRange=12, glide=0, body=0.1299, harmonics=0.4659, harmonicBalance=-0.626, pitchDrop=19.7, pitchDecay=54.3, pitchCurve=1.5, attack=0.2694, hold=0, decay=260, sustain=0.0387, release=150, ampCurve=0.6733, click=0.4581, clickTone=5905.6604, clickDecay=2.3, punch=0.4446, tone=8117.0213, toneKeytrack=0.18, drive=12.2, driveMode=1, compressor=0.3877, clipper=0.4382, output=-4.19),
            R("Paper Route", "A mid-heavy clipped bounce that survives phone playback.", ["mid-heavy", "phone-ready"], voiceMode=0, legato=False, triggerMode=0, velocitySens=72, tune=0, fine=0, bendRange=12, glide=0, body=0.2029, harmonics=0.368, harmonicBalance=-0.8712, pitchDrop=21.5, pitchDecay=55.3, pitchCurve=0.62, attack=0.2968, hold=0, decay=279.2874, sustain=0.0436, release=82, ampCurve=0.4124, click=0.4716, clickTone=6107.409, clickDecay=2.3, punch=0.4615, tone=8520.9479, toneKeytrack=0.18, drive=4.6, driveMode=2, compressor=0.3942, clipper=0.4318, output=-7.14),
            R("Side Door", "A stealthy dry short bass with restrained click and grit.", ["dry", "restrained"], voiceMode=0, legato=False, triggerMode=0, velocitySens=72, tune=0, fine=0, bendRange=12, glide=0, body=0.2488, harmonics=0.5746, harmonicBalance=-0.5444, pitchDrop=23.3, pitchDecay=56.3, pitchCurve=0.62, attack=0.3242, hold=0, decay=463.6, sustain=0.0484, release=92, ampCurve=0.4556, click=0.4851, clickTone=6132.0755, clickDecay=2.3, punch=0.4783, tone=8521.2766, toneKeytrack=0.18, drive=3.4, driveMode=1, compressor=0.4006, clipper=0.4253, output=-5.53),
            R("Quick Cash", "A high-velocity short hit made for fast piano-led beats.", ["high-velocity", "piano-beat"], voiceMode=0, legato=False, triggerMode=0, velocitySens=88, tune=0, fine=0, bendRange=12, glide=0, body=0.3082, harmonics=0.0359, harmonicBalance=-0.5035, pitchDrop=25.1, pitchDecay=57.4, pitchCurve=0.4, attack=0.3516, hold=0, decay=325.1, sustain=0.0533, release=28, ampCurve=0.4352, click=0.4986, clickTone=6187.0225, clickDecay=2.3, punch=0.4951, tone=8723.4043, toneKeytrack=0.18, drive=6.6, driveMode=2, compressor=0.4071, clipper=0.4188, output=-7.78),
            R("Beltline Kick", "A kick-like 808 hybrid with a compact sub finish.", ["kick-hybrid", "compact"], voiceMode=0, legato=False, triggerMode=0, velocitySens=72, tune=0, fine=0, bendRange=12, glide=0, body=0.3676, harmonics=0.0903, harmonicBalance=-0.4627, pitchDrop=7.69, pitchDecay=58.4, pitchCurve=0.5, attack=0.379, hold=8, decay=361.8, sustain=0.0474, release=35, ampCurve=0.3622, click=0.5122, clickTone=6358.4906, clickDecay=2.2, punch=0.512, tone=8925.5319, toneKeytrack=0.18, drive=9.7, driveMode=2, compressor=0.4135, clipper=0.4524, output=-7.19),
        ]),
        ("West Coast", [
            R("Palm Bounce", "A warm round coastal 808 with an easy medium glide.", ["round", "easy-glide"], voiceMode=0, legato=True, triggerMode=0, velocitySens=50, tune=0, fine=0, bendRange=12, glide=122, body=0.1336, harmonics=0.1484, harmonicBalance=0.4189, pitchDrop=6.15, pitchDecay=89.8887, pitchCurve=1.3, attack=0.4831, hold=0, decay=2821.9732, sustain=0.2321, release=390, ampCurve=0.9518, click=0.1609, clickTone=3869.4199, clickDecay=4.5, punch=0.1625, tone=3349.9972, toneKeytrack=0.26, drive=1.99, driveMode=1, compressor=0.3563, clipper=0.1829, output=-5.53),
            R("Chrome Coast", "A polished upper-harmonic coast bass with chrome definition.", ["polished", "defined"], voiceMode=0, legato=True, triggerMode=0, velocitySens=50, tune=0, fine=0, bendRange=12, glide=82, body=0.2789, harmonics=0.1626, harmonicBalance=0.2826, pitchDrop=3.2, pitchDecay=57.332, pitchCurve=1.3, attack=0.4097, hold=0, decay=1487.2, sustain=0.0722, release=230, ampCurve=0.6346, click=0.1595, clickTone=4036.8154, clickDecay=4.5, punch=0.0398, tone=3340.4255, toneKeytrack=0.26, drive=5.88, driveMode=1, compressor=0.2265, clipper=0.2, output=-3.38),
            R("Lowrider Lean", "A slow leaning long-tail 808 for spacious west-coast drums.", ["slow", "long-tail"], voiceMode=0, legato=True, triggerMode=1, velocitySens=50, tune=0, fine=0, bendRange=12, glide=345, body=0.2818, harmonics=0.4177, harmonicBalance=0.3002, pitchDrop=30.77, pitchDecay=56.633, pitchCurve=2, attack=0.4305, hold=0, decay=2495.92, sustain=0.4049, release=820, ampCurve=0.6934, click=0.2113, clickTone=4074.2443, clickDecay=4.5, punch=0.2134, tone=4496.2115, toneKeytrack=0.26, drive=5.96, driveMode=1, compressor=0.2855, clipper=0.2121, output=-8.21),
            R("Sunset Trunk", "A soft sunset trunk tone with a smooth musical release.", ["soft", "musical"], voiceMode=0, legato=True, triggerMode=1, velocitySens=50, tune=0, fine=0, bendRange=12, glide=265, body=0.3093, harmonics=0.3552, harmonicBalance=0.4082, pitchDrop=6.73, pitchDecay=59.4532, pitchCurve=1.55, attack=0.414, hold=0, decay=1353.2888, sustain=0.0576, release=620, ampCurve=0.7384, click=0.2008, clickTone=3525.2719, clickDecay=4.5, punch=0.1915, tone=3200.8078, toneKeytrack=0.26, drive=6.7, driveMode=1, compressor=0.3485, clipper=0.1794, output=-2.6),
            R("G-Funk Floor", "A harmonically rich tonal 808 suited to melodic funk basslines.", ["tonal", "funk"], voiceMode=0, legato=True, triggerMode=1, velocitySens=50, tune=0, fine=0, bendRange=12, glide=182, body=0.3347, harmonics=0.2591, harmonicBalance=0.3931, pitchDrop=6.13, pitchDecay=69.3564, pitchCurve=1.3, attack=0.4677, hold=0, decay=2578, sustain=0.2427, release=470, ampCurve=1.0156, click=0.2383, clickTone=3471.6981, clickDecay=4.5, punch=0.0522, tone=4938.211, toneKeytrack=0.48, drive=1.1, driveMode=1, compressor=0.2299, clipper=0.1847, output=-5.91),
            R("Hydraulic Drop", "A pronounced downward drop with a springy hydraulic tail.", ["drop", "springy"], voiceMode=0, legato=True, triggerMode=0, velocitySens=50, tune=0, fine=0, bendRange=12, glide=142, body=0.0416, harmonics=0.1932, harmonicBalance=0.4192, pitchDrop=8.8, pitchDecay=64.4, pitchCurve=0.72, attack=0.3879, hold=0, decay=1288.1, sustain=0.2165, release=170, ampCurve=0.6068, click=0.2011, clickTone=3432.8882, clickDecay=4.5, punch=0.1767, tone=3339.0415, toneKeytrack=0.26, drive=7.75, driveMode=2, compressor=0.2629, clipper=0.201, output=-2.94),
            R("Boulevard Bass", "A pure boulevard sub that glides beneath busy instrumentation.", ["pure", "under-mix"], voiceMode=0, legato=True, triggerMode=1, velocitySens=50, tune=0, fine=0, bendRange=12, glide=425, body=0.0305, harmonics=0.1819, harmonicBalance=0.3165, pitchDrop=14.18, pitchDecay=114.8878, pitchCurve=2.3, attack=0.4135, hold=0, decay=1475.9198, sustain=0.4155, release=980, ampCurve=0.6566, click=0.1955, clickTone=4020.6624, clickDecay=4.5, punch=0.2384, tone=4467.205, toneKeytrack=0.26, drive=5.8, driveMode=1, compressor=0.2889, clipper=0.1794, output=-2.6),
            R("Coastline Smoke", "A dark smoky legato tone with a soft tube edge.", ["smoky", "tube"], voiceMode=0, legato=True, triggerMode=1, velocitySens=50, tune=0, fine=0, bendRange=12, glide=305, body=0.0968, harmonics=0.2709, harmonicBalance=0.3962, pitchDrop=6.6, pitchDecay=66.5029, pitchCurve=1.3, attack=0.4621, hold=0, decay=1200, sustain=0.0537, release=1050, ampCurve=0.5242, click=0.238, clickTone=3471.69, clickDecay=4.5, punch=0.0701, tone=4920.8129, toneKeytrack=0.26, drive=4.17, driveMode=1, compressor=0.3162, clipper=0.1701, output=-2.04),
            R("Coast Chrome", "A bright modern coast 808 with wide upper-harmonic character.", ["bright", "modern"], voiceMode=0, legato=True, triggerMode=0, velocitySens=50, tune=0, fine=0, bendRange=12, glide=112, body=0.2973, harmonics=0.3864, harmonicBalance=0.3965, pitchDrop=12.2, pitchDecay=111.833, pitchCurve=1.3, attack=0.4086, hold=0, decay=2156.1, sustain=0.16, release=330, ampCurve=0.6759, click=0.1913, clickTone=3347.8292, clickDecay=4.5, punch=0.1791, tone=3185.5734, toneKeytrack=0.4, drive=6.89, driveMode=2, compressor=0.3316, clipper=0.1707, output=-6.43),
            R("Sunday Cruise", "A relaxed cruising 808 with a gentle pitch envelope.", ["relaxed", "gentle"], voiceMode=0, legato=True, triggerMode=1, velocitySens=50, tune=0, fine=0, bendRange=12, glide=225, body=0.2174, harmonics=0.3421, harmonicBalance=0.4157, pitchDrop=13.3, pitchDecay=72.2577, pitchCurve=2.1, attack=0.497, hold=0, decay=1397.8932, sustain=0.2253, release=780, ampCurve=0.6926, click=0.1594, clickTone=3987.3214, clickDecay=4.5, punch=0.1807, tone=3286.0395, toneKeytrack=0.26, drive=1.6, driveMode=1, compressor=0.3539, clipper=0.1818, output=-4.48),
            R("Daylight Rider", "A clean daylight bass with extra melodic key tracking.", ["clean", "melodic"], voiceMode=1, legato=False, triggerMode=1, velocitySens=50, tune=0, fine=0, bendRange=12, glide=0, body=0.0451, harmonics=0.1962, harmonicBalance=0.4304, pitchDrop=27.96, pitchDecay=49.6208, pitchCurve=1.3, attack=0.4686, hold=0, decay=1353.2886, sustain=0.2738, release=540, ampCurve=0.5168, click=0.246, clickTone=3624.1137, clickDecay=4.5, punch=0.0666, tone=3453.4833, toneKeytrack=0.62, drive=4.17, driveMode=1, compressor=0.2949, clipper=0.2, output=-0.24),
            R("Golden State Knock", "A firm west-coast knock with warm saturation and short glide.", ["firm", "warm"], voiceMode=0, legato=True, triggerMode=0, velocitySens=50, tune=0, fine=0, bendRange=12, glide=68, body=0.042, harmonics=0.1932, harmonicBalance=0.4082, pitchDrop=26.55, pitchDecay=114.2095, pitchCurve=1.3, attack=0.416, hold=0, decay=1395.8, sustain=0.2495, release=130, ampCurve=0.6507, click=0.1783, clickTone=3989.1854, clickDecay=4.5, punch=0.1465, tone=3200.8076, toneKeytrack=0.26, drive=8.07, driveMode=1, compressor=0.3565, clipper=0.2, output=-3.61),
        ]),
        ("Long Glide", [
            R("Endless Bend", "An extremely long clean bend with an almost continuous tail.", ["extreme-glide", "clean"], voiceMode=0, legato=True, triggerMode=1, velocitySens=46, tune=0, fine=0, bendRange=24, glide=720, body=0.0639, harmonics=0.0585, harmonicBalance=-0.5242, pitchDrop=9.5, pitchDecay=84.48, pitchCurve=1.45, attack=0.4526, hold=0, decay=4629.5486, sustain=0.7984, release=1250, ampCurve=0.7696, click=0.0613, clickTone=2737.1605, clickDecay=5.5, punch=0.0694, tone=1787.9515, toneKeytrack=0.28, drive=3.61, driveMode=1, compressor=0.0714, clipper=0.0594, output=-5.03),
            R("Silk Slide", "A smooth silky legato slide with soft even harmonics.", ["smooth", "even-harmonic"], voiceMode=0, legato=True, triggerMode=1, velocitySens=46, tune=0, fine=0, bendRange=24, glide=525, body=0.125, harmonics=0.1946, harmonicBalance=-0.7329, pitchDrop=11.6, pitchDecay=79.8217, pitchCurve=1.45, attack=0.3985, hold=0, decay=2907.5533, sustain=0.7019, release=980, ampCurve=1.0687, click=0.1158, clickTone=2118.5918, clickDecay=5.5, punch=0.0388, tone=1776.5957, toneKeytrack=0.28, drive=1.32, driveMode=1, compressor=0.1082, clipper=0.0162, output=-4.67),
            R("Lunar Portamento", "A wide tonal portamento preset for dramatic octave jumps.", ["portamento", "octave"], voiceMode=0, legato=True, triggerMode=1, velocitySens=46, tune=0, fine=0, bendRange=24, glide=920, body=0.1916, harmonics=0.1756, harmonicBalance=-0.9096, pitchDrop=27.56, pitchDecay=81.6321, pitchCurve=2.4, attack=0.4043, hold=0, decay=4403.6, sustain=0.7543, release=1500, ampCurve=0.6956, click=0.117, clickTone=2254.717, clickDecay=5.5, punch=0.1225, tone=1914.8936, toneKeytrack=0.55, drive=9, driveMode=2, compressor=0.1407, clipper=0.0121, output=-7.77),
            R("Slow Lane", "A pure slow-lane sub with understated note transitions.", ["pure", "understated"], voiceMode=0, legato=True, triggerMode=1, velocitySens=46, tune=0, fine=0, bendRange=24, glide=625, body=0.274, harmonics=0.5532, harmonicBalance=-0.8147, pitchDrop=27.17, pitchDecay=70.433, pitchCurve=1.9, attack=0.3686, hold=0, decay=3806.8884, sustain=0.7508, release=1150, ampCurve=1.3751, click=0.1054, clickTone=2305.3843, clickDecay=5.5, punch=0.0175, tone=2423.8969, toneKeytrack=0.28, drive=2.16, driveMode=1, compressor=0.07, clipper=0.0384, output=-5.46),
            R("Deep Curve", "A dark curved glide with enough harmonics to remain audible.", ["dark", "audible"], voiceMode=0, legato=True, triggerMode=1, velocitySens=46, tune=0, fine=0, bendRange=24, glide=465, body=0.0897, harmonics=0.2332, harmonicBalance=-0.683, pitchDrop=24.57, pitchDecay=76.6548, pitchCurve=1.45, attack=0.3884, hold=0, decay=2694.4, sustain=0.728, release=820, ampCurve=0.8349, click=0.1118, clickTone=2100.2299, clickDecay=5.5, punch=0.0235, tone=2825.4106, toneKeytrack=0.28, drive=11.23, driveMode=1, compressor=0.1174, clipper=0.0973, output=-3.44),
            R("Serpent Tail", "A winding saturated tail with pronounced odd-harmonic motion.", ["winding", "odd-harmonic"], voiceMode=0, legato=True, triggerMode=1, velocitySens=46, tune=0, fine=0, bendRange=24, glide=785, body=0.0594, harmonics=0.4685, harmonicBalance=-0.7278, pitchDrop=13.14, pitchDecay=70.6, pitchCurve=2.6, attack=0.3194, hold=0, decay=3201.6, sustain=0.7258, release=1080, ampCurve=1.2052, click=0.0789, clickTone=2509.434, clickDecay=5.5, punch=0.0707, tone=2329.7872, toneKeytrack=0.28, drive=11.38, driveMode=3, compressor=0.0385, clipper=0.034, output=-7.18),
            R("Ghost Glide", "A nearly invisible fundamental-only glide for layering.", ["fundamental", "layering"], voiceMode=0, legato=True, triggerMode=1, velocitySens=46, tune=0, fine=0, bendRange=24, glide=860, body=0.3142, harmonics=0.1591, harmonicBalance=-0.5422, pitchDrop=15.37, pitchDecay=49.733, pitchCurve=1, attack=0.3664, hold=0, decay=3690, sustain=0.6471, release=1600, ampCurve=1.3345, click=0.0717, clickTone=2594.3396, clickDecay=5.5, punch=0.0883, tone=2468.0851, toneKeytrack=0.28, drive=9.67, driveMode=1, compressor=0.0402, clipper=0.0336, output=-5.99),
            R("Late Arrival", "A delayed-feeling pitch fall followed by a medium-long glide.", ["delayed-pitch", "medium-glide"], voiceMode=0, legato=True, triggerMode=1, velocitySens=46, tune=0, fine=0, bendRange=24, glide=385, body=0.0615, harmonics=0.4684, harmonicBalance=-0.7648, pitchDrop=7.96, pitchDecay=79.97, pitchCurve=1.8, attack=0.2388, hold=0, decay=3896.4368, sustain=0.6455, release=740, ampCurve=1.0934, click=0.0698, clickTone=2473.8148, clickDecay=5.5, punch=0.0847, tone=1912.6448, toneKeytrack=0.28, drive=9.99, driveMode=1, compressor=0.118, clipper=0.0649, output=-6.58),
            R("Infinite Trunk", "A massive sustained trunk note with reliable mono legato behavior.", ["massive", "trunk"], voiceMode=0, legato=True, triggerMode=1, velocitySens=46, tune=0, fine=0, bendRange=24, glide=645, body=0.2268, harmonics=0.0459, harmonicBalance=-0.8147, pitchDrop=24.79, pitchDecay=108.6661, pitchCurve=2.2, attack=0.3377, hold=0, decay=4727.5535, sustain=0.8035, release=1800, ampCurve=0.771, click=0.1041, clickTone=2262.5172, clickDecay=5.5, punch=0.0175, tone=3089.7509, toneKeytrack=0.28, drive=5.81, driveMode=1, compressor=0.0741, clipper=0.0616, output=-8.52),
            R("Afterhours Drift", "A harmonically rich after-hours drift with a relaxed release.", ["rich", "relaxed"], voiceMode=0, legato=True, triggerMode=1, velocitySens=46, tune=0, fine=0, bendRange=24, glide=565, body=0.2315, harmonics=0.5855, harmonicBalance=-0.701, pitchDrop=14, pitchDecay=58.6767, pitchCurve=1.45, attack=0.3121, hold=0, decay=4800, sustain=0.6715, release=1100, ampCurve=0.787, click=0.0937, clickTone=3000.506, clickDecay=5.5, punch=0.0624, tone=2770.5412, toneKeytrack=0.28, drive=5.71, driveMode=2, compressor=0.107, clipper=0.0887, output=-7.89),
            R("Long Game", "A practical long glide tuned for sustained modern bass melodies.", ["practical", "bass-melody"], voiceMode=0, legato=True, triggerMode=1, velocitySens=46, tune=0, fine=0, bendRange=24, glide=440, body=0.2741, harmonics=0.0414, harmonicBalance=-0.5471, pitchDrop=12.96, pitchDecay=101.0878, pitchCurve=1.45, attack=0.4486, hold=0, decay=2502.66, sustain=0.7016, release=920, ampCurve=0.633, click=0.0601, clickTone=2907.1098, clickDecay=5.5, punch=0.087, tone=2618.4137, toneKeytrack=0.42, drive=2.76, driveMode=1, compressor=0.1232, clipper=0.0167, output=-7.31),
            R("Gravity Lane", "A heavy downward glide with a pronounced opening pitch drop.", ["heavy", "downward"], voiceMode=0, legato=True, triggerMode=1, velocitySens=46, tune=0, fine=0, bendRange=24, glide=690, body=0.0631, harmonics=0.4365, harmonicBalance=-0.6101, pitchDrop=16.8, pitchDecay=95.2, pitchCurve=1.65, attack=0.4587, hold=0, decay=2600.6, sustain=0.839, release=1300, ampCurve=0.9961, click=0.062, clickTone=2890.2667, clickDecay=5.5, punch=0.0707, tone=2590.9789, toneKeytrack=0.28, drive=8.55, driveMode=1, compressor=0.1381, clipper=0.029, output=-8.82),
        ]),
        ("Short Punch", [
            R("Kickback", "A compact kickback hit with a hard descending transient.", ["compact", "hard"], voiceMode=0, legato=False, triggerMode=0, velocitySens=78, tune=0, fine=0, bendRange=12, glide=0, body=0.0756, harmonics=0.0638, harmonicBalance=-0.8617, pitchDrop=9.6, pitchDecay=46, pitchCurve=0.42, attack=0.0774, hold=4, decay=352.3, sustain=0.0016, release=24, ampCurve=0.3933, click=0.3635, clickTone=5132.0755, clickDecay=1, punch=0.3717, tone=7191.4894, toneKeytrack=0.08, drive=3.9, driveMode=2, compressor=0.0508, clipper=0.0224, output=1.03),
            R("Chest Tap", "A round chest-level thump with a slightly softer click.", ["round", "thump"], voiceMode=0, legato=False, triggerMode=0, velocitySens=78, tune=0, fine=0, bendRange=12, glide=0, body=0.1511, harmonics=0.1275, harmonicBalance=-0.8209, pitchDrop=11, pitchDecay=46, pitchCurve=0.5, attack=0.1048, hold=4, decay=213.8, sustain=0.0032, release=45, ampCurve=0.5067, click=0.377, clickTone=5264.1509, clickDecay=2.8, punch=0.3874, tone=7382.9787, toneKeytrack=0.08, drive=7.8, driveMode=1, compressor=0.0815, clipper=0.0447, output=0.59),
            R("Quick Knock", "An ultra-short high-impact knock for rapid patterns.", ["ultra-short", "rapid"], voiceMode=0, legato=False, triggerMode=0, velocitySens=78, tune=0, fine=0, bendRange=12, glide=0, body=0.2267, harmonics=0.1913, harmonicBalance=-0.7801, pitchDrop=12.5, pitchDecay=46, pitchCurve=0.27, attack=0.1323, hold=2, decay=490.8, sustain=0.0047, release=12, ampCurve=0.3178, click=0.3905, clickTone=5396.2264, clickDecay=0.55, punch=0.4031, tone=7574.4681, toneKeytrack=0.08, drive=11.7, driveMode=3, compressor=0.1123, clipper=0.0671, output=-0.68),
            R("Tight Pocket", "A disciplined tight 808 that stays clear around fast kicks.", ["disciplined", "clear"], voiceMode=0, legato=False, triggerMode=0, velocitySens=78, tune=0, fine=0, bendRange=12, glide=0, body=0.3023, harmonics=0.2551, harmonicBalance=-0.7392, pitchDrop=14, pitchDecay=46, pitchCurve=0.5, attack=0.1597, hold=4, decay=144.6, sustain=0.0063, release=62, ampCurve=0.4311, click=0.4041, clickTone=5528.3019, clickDecay=1.8, punch=0.4188, tone=7765.9574, toneKeytrack=0.08, drive=15.6, driveMode=1, compressor=0.1431, clipper=0.0894, output=0.86),
            R("Staccato Sub", "A pure staccato sub with almost no upper harmonic content.", ["pure", "staccato"], voiceMode=0, legato=False, triggerMode=0, velocitySens=78, tune=0, fine=0, bendRange=12, glide=0, body=0.3779, harmonics=0.3189, harmonicBalance=-0.6984, pitchDrop=15.4, pitchDecay=46, pitchCurve=0.9, attack=0.1871, hold=0, decay=322.4611, sustain=0.0079, release=38, ampCurve=0.4977, click=0.4176, clickTone=5475.0422, clickDecay=1.8, punch=0.4345, tone=7957.4468, toneKeytrack=0.08, drive=0.8, driveMode=2, compressor=0.1738, clipper=0.1118, output=1.21),
            R("One Two", "A two-part punch with bright click followed by a thick body.", ["two-part", "bright"], voiceMode=0, legato=False, triggerMode=0, velocitySens=78, tune=0, fine=0, bendRange=12, glide=0, body=0.4534, harmonics=0.3826, harmonicBalance=-0.6576, pitchDrop=16.9, pitchDecay=46, pitchCurve=0.5, attack=0.2145, hold=4, decay=283.1, sustain=0.0095, release=78, ampCurve=0.3556, click=0.4311, clickTone=5792.4528, clickDecay=2.5, punch=0.4502, tone=8148.9362, toneKeytrack=0.08, drive=4.7, driveMode=2, compressor=0.2046, clipper=0.1341, output=-3.01),
            R("Short Fuse", "An explosive clipped micro-tail for rage-style bass rhythms.", ["explosive", "rage"], voiceMode=0, legato=False, triggerMode=0, velocitySens=78, tune=0, fine=0, bendRange=12, glide=0, body=0.0108, harmonics=0.4464, harmonicBalance=-0.6168, pitchDrop=18.4, pitchDecay=46, pitchCurve=0.31, attack=0.2419, hold=1, decay=560, sustain=0.0111, release=10, ampCurve=0.4689, click=0.4446, clickTone=5924.5283, clickDecay=0.45, punch=0.466, tone=8340.4255, toneKeytrack=0.08, drive=8.6, driveMode=3, compressor=0.2354, clipper=0.1565, output=-3.5),
            R("Snap Weight", "A snapping attack paired with a medium-short weighty tail.", ["snap", "weighty"], voiceMode=0, legato=False, triggerMode=0, velocitySens=78, tune=0, fine=0, bendRange=12, glide=0, body=0.0864, harmonics=0.5102, harmonicBalance=-0.576, pitchDrop=19.9, pitchDecay=46, pitchCurve=0.5, attack=0.2694, hold=4, decay=110, sustain=0.0126, release=92, ampCurve=0.5822, click=0.4581, clickTone=6056.6038, clickDecay=1.8, punch=0.4817, tone=8531.9149, toneKeytrack=0.08, drive=12.5, driveMode=2, compressor=0.2662, clipper=0.1788, output=-2.49),
            R("Pocket Hammer", "A dense hammer strike that remains centered and mono-safe.", ["dense", "mono-safe"], voiceMode=0, legato=False, triggerMode=0, velocitySens=78, tune=0, fine=0, bendRange=12, glide=0, body=0.1619, harmonics=0.574, harmonicBalance=-0.5352, pitchDrop=21.3, pitchDecay=46, pitchCurve=0.5, attack=0.2968, hold=4, decay=386.9, sustain=0.0142, release=48, ampCurve=0.2926, click=0.4716, clickTone=6188.6792, clickDecay=1.4, punch=0.4974, tone=8723.4043, toneKeytrack=0.08, drive=16.4, driveMode=2, compressor=0.2969, clipper=0.2012, output=-2.3),
            R("Dry Impact", "A dry woody impact with minimal saturation and no lingering tail.", ["dry", "woody"], voiceMode=0, legato=False, triggerMode=0, velocitySens=78, tune=0, fine=0, bendRange=12, glide=0, body=0.2375, harmonics=0.6377, harmonicBalance=-0.4944, pitchDrop=22.8, pitchDecay=46, pitchCurve=0.5, attack=0.3242, hold=4, decay=248.5, sustain=0.0158, release=20, ampCurve=0.4059, click=0.4851, clickTone=6320.7547, clickDecay=3.8, punch=0.5131, tone=8914.8936, toneKeytrack=0.08, drive=1.6, driveMode=0, compressor=0.3277, clipper=0.2235, output=-3.16),
            R("Fast Cut", "A high-passed-feeling short cut that leaves maximum bass headroom.", ["fast", "headroom"], voiceMode=0, legato=False, triggerMode=0, velocitySens=78, tune=0, fine=0, bendRange=12, glide=0, body=0.3131, harmonics=0.0058, harmonicBalance=-0.4535, pitchDrop=24.3, pitchDecay=46, pitchCurve=0.5, attack=0.3516, hold=4, decay=525.4, sustain=0.0174, release=28, ampCurve=0.5193, click=0.4986, clickTone=6452.8302, clickDecay=0.9, punch=0.5288, tone=9106.383, toneKeytrack=0.08, drive=5.5, driveMode=2, compressor=0.3585, clipper=0.2459, output=-5.71),
            R("Mini Monster", "A tiny tail with oversized harmonics and hard clip character.", ["tiny-tail", "oversized"], voiceMode=0, legato=False, triggerMode=0, velocitySens=78, tune=0, fine=0, bendRange=12, glide=0, body=0.3887, harmonics=0.0696, harmonicBalance=-0.4127, pitchDrop=25.7, pitchDecay=46, pitchCurve=0.5, attack=0.379, hold=4, decay=179.2, sustain=0.0189, release=16, ampCurve=0.3304, click=0.5122, clickTone=6584.9057, clickDecay=1.1, punch=0.5445, tone=9297.8723, toneKeytrack=0.08, drive=9.4, driveMode=3, compressor=0.3892, clipper=0.2682, output=-5.7),
        ]),
        ("Experimental", [
            R("Zero Gravity", "A floating polyphonic bass texture with slow pitch movement.", ["polyphonic", "floating"], voiceMode=1, legato=False, triggerMode=1, velocitySens=64, tune=0, fine=-5, bendRange=24, glide=0, body=0.1429, harmonics=0.31, harmonicBalance=-0.9535, pitchDrop=14.98, pitchDecay=68.5104, pitchCurve=3.2, attack=0.2515, hold=20, decay=377.0873, sustain=0.0508, release=760, ampCurve=0.6027, click=0.2586, clickTone=4272.7795, clickDecay=8, punch=0.0244, tone=2744.6809, toneKeytrack=0.55, drive=15.8, driveMode=3, compressor=0.3997, clipper=0.6388, output=-10.92),
            R("Phase Beast", "A high-resonance harmonic beast with extreme portamento.", ["resonant", "extreme"], voiceMode=0, legato=True, triggerMode=1, velocitySens=64, tune=0, fine=0, bendRange=24, glide=980, body=0.2857, harmonics=0.379, harmonicBalance=-0.907, pitchDrop=4.2, pitchDecay=102.9, pitchCurve=3.6, attack=0.1048, hold=20, decay=1123.1, sustain=0.1017, release=470, ampCurve=1.06, click=0.1324, clickTone=2858.4906, clickDecay=8, punch=0.0488, tone=2989.3617, toneKeytrack=0.7, drive=20.8, driveMode=3, compressor=0.4014, clipper=0.6226, output=-10.68),
            R("Bent Glass", "A glassy poly bass with detuning and a slow exaggerated pitch fall.", ["glassy", "detuned"], voiceMode=1, legato=False, triggerMode=1, velocitySens=64, tune=0, fine=12, bendRange=24, glide=0, body=0.6966, harmonics=0.2776, harmonicBalance=-0.8605, pitchDrop=21.36, pitchDecay=161.0722, pitchCurve=3.8, attack=0.2778, hold=35, decay=3584.6, sustain=0.2076, release=980, ampCurve=0.41, click=0.1486, clickTone=3037.7358, clickDecay=11, punch=0.0732, tone=3234.0426, toneKeytrack=0.55, drive=23.31, driveMode=2, compressor=0.4031, clipper=0.4688, output=-12.96),
            R("Rubber Room", "A rubbery slow-attack bass that bends dramatically between notes.", ["rubbery", "slow-attack"], voiceMode=0, legato=True, triggerMode=1, velocitySens=64, tune=0, fine=0, bendRange=24, glide=835, body=0.5714, harmonics=0.517, harmonicBalance=-0.814, pitchDrop=8.3, pitchDecay=130.9, pitchCurve=3.4, attack=0.1597, hold=45, decay=2705, sustain=0.3653, release=820, ampCurve=0.8, click=0.1649, clickTone=3448.5425, clickDecay=8, punch=0.0976, tone=3159.4447, toneKeytrack=0.55, drive=30.9, driveMode=3, compressor=0.4014, clipper=0.5189, output=-7.26),
            R("Alien Trunk", "An extreme alien one-shot with folding highs and maximum pitch drop.", ["alien", "one-shot"], voiceMode=0, legato=False, triggerMode=0, velocitySens=64, tune=0, fine=0, bendRange=24, glide=0, body=0.7143, harmonics=0.586, harmonicBalance=-0.7674, pitchDrop=10.4, pitchDecay=144.8, pitchCurve=4, attack=0.1871, hold=20, decay=3616.4079, sustain=0.2542, release=210, ampCurve=0.4132, click=0.1811, clickTone=3396.2264, clickDecay=18, punch=0.122, tone=3723.4043, toneKeytrack=0.55, drive=18.32, driveMode=3, compressor=0.4065, clipper=0.5741, output=-15.42),
            R("Octave Ghost", "A ghostly sub transposed down an octave with a huge sustained tail.", ["octave-down", "ghostly"], voiceMode=0, legato=True, triggerMode=1, velocitySens=64, tune=-12, fine=-3, bendRange=24, glide=1100, body=0.8571, harmonics=0.655, harmonicBalance=-0.7209, pitchDrop=12.5, pitchDecay=158.8, pitchCurve=3, attack=0.2145, hold=20, decay=1738.5, sustain=0.3051, release=1500, ampCurve=0.54, click=0.1973, clickTone=3575.4717, clickDecay=8, punch=0.1463, tone=3968.0851, toneKeytrack=0.55, drive=16.8, driveMode=1, compressor=0.4082, clipper=0.5579, output=-12.37),
            R("Circuit Melt", "A melting circuit tone with dense odd harmonics and long glide.", ["melting", "odd-harmonic"], voiceMode=0, legato=True, triggerMode=1, velocitySens=64, tune=0, fine=0, bendRange=24, glide=620, body=0.0204, harmonics=0.724, harmonicBalance=-0.6744, pitchDrop=14.6, pitchDecay=172.8, pitchCurve=2.4, attack=0.2419, hold=20, decay=4200, sustain=0.3559, release=370, ampCurve=0.93, click=0.2135, clickTone=3754.717, clickDecay=7, punch=0.1707, tone=4212.766, toneKeytrack=0.55, drive=21.8, driveMode=3, compressor=0.4098, clipper=0.5418, output=-10.79),
            R("Hollow Square", "A hollow square-like bass with even harmonics and gate control.", ["hollow", "square-like"], voiceMode=0, legato=True, triggerMode=1, velocitySens=64, tune=0, fine=0, bendRange=24, glide=430, body=0.1633, harmonics=0.793, harmonicBalance=-0.6279, pitchDrop=16.7, pitchDecay=186.7, pitchCurve=2.7, attack=0.2694, hold=20, decay=200, sustain=0.4067, release=930, ampCurve=1.32, click=0.2297, clickTone=3933.9623, clickDecay=8, punch=0.1951, tone=4457.4468, toneKeytrack=0.55, drive=26.9, driveMode=2, compressor=0.4115, clipper=0.5256, output=-12.66),
            R("Radioactive Sub", "A radioactive resonant sub with slow attack and unstable color.", ["radioactive", "resonant"], voiceMode=0, legato=True, triggerMode=1, velocitySens=64, tune=0, fine=0, bendRange=24, glide=760, body=0.3061, harmonics=0.862, harmonicBalance=-0.9074, pitchDrop=18.8, pitchDecay=200.7, pitchCurve=3.5, attack=0.2968, hold=20, decay=2661.5, sustain=0.4576, release=1180, ampCurve=0.3233, click=0.2265, clickTone=4113.2075, clickDecay=8, punch=0.2536, tone=4702.1277, toneKeytrack=0.82, drive=32, driveMode=3, compressor=0.4132, clipper=0.5094, output=-6.05),
            R("Reverse Gravity", "A slow-settling pitch illusion with maximal expressive glide.", ["pitch-illusion", "expressive"], voiceMode=0, legato=True, triggerMode=1, velocitySens=64, tune=0, fine=0, bendRange=24, glide=1250, body=0.449, harmonics=0.931, harmonicBalance=-0.5349, pitchDrop=30.99, pitchDecay=59.1217, pitchCurve=4, attack=0.2119, hold=80, decay=1430.8, sustain=0.1517, release=1050, ampCurve=0.7133, click=0.2622, clickTone=4292.4528, clickDecay=14, punch=0.2439, tone=4946.8085, toneKeytrack=0.55, drive=12.7, driveMode=2, compressor=0.4149, clipper=0.4932, output=-12.78),
            R("Broken Orbit", "A polyphonic octave-up effect bass with a fractured short envelope.", ["octave-up", "fractured"], voiceMode=1, legato=False, triggerMode=0, velocitySens=64, tune=12, fine=7, bendRange=24, glide=0, body=0.5918, harmonics=0.2473, harmonicBalance=-0.4884, pitchDrop=23, pitchDecay=228.6, pitchCurve=0.35, attack=0.3516, hold=12, decay=3892.3, sustain=0.5593, release=90, ampCurve=1.1033, click=0.2784, clickTone=4471.6981, clickDecay=2.6, punch=0.2683, tone=5191.4894, toneKeytrack=0.55, drive=17.8, driveMode=3, compressor=0.4166, clipper=0.4771, output=-14.05),
            R("Laser Basement", "A laser-like high click over a dark sustained basement sub.", ["laser", "dark-sub"], voiceMode=0, legato=True, triggerMode=1, velocitySens=64, tune=0, fine=0, bendRange=24, glide=350, body=0.7347, harmonics=0.3163, harmonicBalance=-0.4419, pitchDrop=25, pitchDecay=242.6, pitchCurve=2.8, attack=0.379, hold=20, decay=815.4, sustain=0.6101, release=1350, ampCurve=0.4533, click=0.2946, clickTone=4650.9434, clickDecay=22, punch=0.2927, tone=5436.1702, toneKeytrack=0.55, drive=22.8, driveMode=1, compressor=0.4183, clipper=0.4609, output=-13.12),
        ]),
        ("Mix Ready", [
            R("Vocal Pocket", "A controlled dark 808 shaped to leave space for lead vocals.", ["vocal-space", "dark"], voiceMode=0, legato=False, triggerMode=0, velocitySens=58, tune=0, fine=0, bendRange=12, glide=0, body=0.0371, harmonics=0.0883, harmonicBalance=0.2927, pitchDrop=14.85, pitchDecay=50.0542, pitchCurve=0.95, attack=0.2976, hold=0, decay=646.2, sustain=0.0133, release=180, ampCurve=0.5552, click=0.303, clickTone=4861.8269, clickDecay=3, punch=0.2963, tone=4170.2128, toneKeytrack=0.14, drive=1.8, driveMode=1, compressor=0.1875, clipper=0.0224, output=1.55),
            R("Small Speaker Safe", "A harmonic-balanced 808 that remains audible on small speakers.", ["small-speaker", "audible"], voiceMode=0, legato=False, triggerMode=0, velocitySens=58, tune=0, fine=0, bendRange=12, glide=0, body=0.1043, harmonics=0.0703, harmonicBalance=-0.0543, pitchDrop=14.85, pitchDecay=47.8, pitchCurve=0.95, attack=0.3912, hold=0, decay=1368.6917, sustain=0.0913, release=120, ampCurve=0.4236, click=0.3661, clickTone=4592.1303, clickDecay=3, punch=0.1588, tone=4340.4255, toneKeytrack=0.14, drive=3.7, driveMode=2, compressor=0.3561, clipper=0.0741, output=-1.05),
            R("Master Bus Safe", "A conservative clean sub with ample mastering headroom.", ["headroom", "clean"], voiceMode=0, legato=False, triggerMode=0, velocitySens=58, tune=0, fine=0, bendRange=12, glide=0, body=0.1469, harmonics=0.2686, harmonicBalance=0.0434, pitchDrop=17.24, pitchDecay=59.685, pitchCurve=0.95, attack=0.2202, hold=0, decay=949.2, sustain=0.0542, release=250, ampCurve=0.4267, click=0.2283, clickTone=3756.9026, clickDecay=3, punch=0.1445, tone=5605.887, toneKeytrack=0.14, drive=2.68, driveMode=0, compressor=0.0969, clipper=0.0671, output=0.91),
            R("Club Translation", "A club-balanced 808 with firm fundamental and audible mids.", ["club", "balanced"], voiceMode=0, legato=True, triggerMode=0, velocitySens=58, tune=0, fine=0, bendRange=12, glide=72, body=0.2058, harmonics=0.0186, harmonicBalance=-0.0041, pitchDrop=12.15, pitchDecay=53.9516, pitchCurve=0.95, attack=0.1749, hold=0, decay=639.8447, sustain=0.0241, release=205, ampCurve=0.539, click=0.2432, clickTone=3915.0943, clickDecay=3, punch=0.1475, tone=4680.8511, toneKeytrack=0.14, drive=4.41, driveMode=1, compressor=0.1292, clipper=0.0894, output=-0.42),
            R("Streaming Low", "A compact level-controlled low end prepared for streaming masters.", ["streaming", "level-controlled"], voiceMode=0, legato=False, triggerMode=0, velocitySens=58, tune=0, fine=0, bendRange=12, glide=0, body=0.1373, harmonics=0.0854, harmonicBalance=0.115, pitchDrop=11.9, pitchDecay=58.345, pitchCurve=0.95, attack=0.1551, hold=0, decay=1277.2281, sustain=0.0666, release=145, ampCurve=0.3739, click=0.344, clickTone=4481.6679, clickDecay=3, punch=0.1238, tone=4885.9742, toneKeytrack=0.14, drive=2.33, driveMode=2, compressor=0.2342, clipper=0.2143, output=-4.11),
            R("Tight Master", "A tight clipped body that stays disciplined under bus limiting.", ["tight", "limiter-safe"], voiceMode=0, legato=False, triggerMode=0, velocitySens=58, tune=0, fine=0, bendRange=12, glide=0, body=0.1774, harmonics=0.3194, harmonicBalance=0.276, pitchDrop=12.6, pitchDecay=51.4, pitchCurve=0.66, attack=0.4808, hold=0, decay=694.1, sustain=0.0799, release=88, ampCurve=0.5857, click=0.2545, clickTone=4122.6415, clickDecay=3, punch=0.2142, tone=5021.2766, toneKeytrack=0.14, drive=3.79, driveMode=3, compressor=0.3334, clipper=0.1341, output=-1.85),
            R("Clean Loud", "A clean loudness-optimized 808 without excessive saturation.", ["loud", "clean"], voiceMode=0, legato=False, triggerMode=0, velocitySens=58, tune=0, fine=0, bendRange=12, glide=0, body=0.1606, harmonics=0.246, harmonicBalance=0.2211, pitchDrop=19.97, pitchDecay=50.0542, pitchCurve=0.95, attack=0.2976, hold=0, decay=646.2, sustain=0.0871, release=165, ampCurve=0.4288, click=0.2757, clickTone=4861.8268, clickDecay=3, punch=0.248, tone=4258.1491, toneKeytrack=0.14, drive=2.91, driveMode=2, compressor=0.2634, clipper=0.0776, output=-3.17),
            R("Sidechain Space", "A shorter release 808 designed to recover cleanly after ducking.", ["sidechain", "short-release"], voiceMode=0, legato=True, triggerMode=0, velocitySens=58, tune=0, fine=0, bendRange=12, glide=122, body=0.0467, harmonics=0.2024, harmonicBalance=-0.0042, pitchDrop=15.39, pitchDecay=57.5982, pitchCurve=0.95, attack=0.3249, hold=0, decay=1417.3835, sustain=0.0698, release=95, ampCurve=0.4564, click=0.3419, clickTone=4622.0153, clickDecay=3, punch=0.2178, tone=5141.2932, toneKeytrack=0.14, drive=2.28, driveMode=1, compressor=0.3171, clipper=0.0353, output=-1.54),
            R("Mono Certified", "A pure centered low end with minimal phase-sensitive harmonics.", ["mono", "phase-safe"], voiceMode=0, legato=False, triggerMode=0, velocitySens=58, tune=0, fine=0, bendRange=12, glide=0, body=0.1096, harmonics=0.3162, harmonicBalance=0.0434, pitchDrop=12.04, pitchDecay=50.0633, pitchCurve=0.95, attack=0.2987, hold=0, decay=636, sustain=0.0221, release=225, ampCurve=0.3689, click=0.3519, clickTone=4846.5253, clickDecay=3, punch=0.1445, tone=5605.8874, toneKeytrack=0.14, drive=7.7, driveMode=1, compressor=0.2908, clipper=0.2933, output=-2.35),
            R("Ready Print", "A finished assertive 808 requiring minimal additional processing.", ["finished", "assertive"], voiceMode=0, legato=True, triggerMode=0, velocitySens=58, tune=0, fine=0, bendRange=12, glide=62, body=0.1467, harmonics=0.3514, harmonicBalance=0.1464, pitchDrop=9.23, pitchDecay=50.6566, pitchCurve=0.58, attack=0.3655, hold=0, decay=893.6141, sustain=0.1748, release=78, ampCurve=0.7101, click=0.3458, clickTone=4537.7358, clickDecay=3, punch=0.1426, tone=5702.1277, toneKeytrack=0.14, drive=6.56, driveMode=2, compressor=0.3231, clipper=0.2235, output=-5.17),
            R("Vocal Gap", "A scooped low-mid 808 with smooth sustain beneath dense vocals.", ["scooped", "vocal-space"], voiceMode=0, legato=True, triggerMode=1, velocitySens=58, tune=0, fine=0, bendRange=12, glide=155, body=0.0653, harmonics=0.0032, harmonicBalance=0.1715, pitchDrop=16.6, pitchDecay=55.9, pitchCurve=0.95, attack=0.3629, hold=0, decay=656.0752, sustain=0.0698, release=360, ampCurve=0.8089, click=0.3189, clickTone=4901.8731, clickDecay=3, punch=0.2673, tone=5677.0297, toneKeytrack=0.14, drive=7.32, driveMode=1, compressor=0.3453, clipper=0.0468, output=-6.91),
            R("Phone Weight", "A carefully driven 808 with second harmonics that survive phone playback.", ["phone", "second-harmonic"], voiceMode=0, legato=False, triggerMode=0, velocitySens=58, tune=0, fine=0, bendRange=12, glide=0, body=0.0897, harmonics=0.0383, harmonicBalance=0.2797, pitchDrop=20.58, pitchDecay=56.8, pitchCurve=0.95, attack=0.3898, hold=0, decay=780, sustain=0.1656, release=130, ampCurve=0.8819, click=0.3297, clickTone=4745.283, clickDecay=3, punch=0.2884, tone=4655.7527, toneKeytrack=0.3, drive=0.46, driveMode=1, compressor=0.3877, clipper=0.1988, output=-2.65),
            R("Car Check", "A car-system-focused 808 with controlled long-bass resonance.", ["car", "long"], voiceMode=0, legato=True, triggerMode=0, velocitySens=58, tune=0, fine=0, bendRange=12, glide=95, body=0.0371, harmonics=0.0883, harmonicBalance=0.0773, pitchDrop=10.85, pitchDecay=57.7, pitchCurve=0.95, attack=0.4166, hold=0, decay=1489.7, sustain=0.0601, release=290, ampCurve=0.5857, click=0.3649, clickTone=4792.138, clickDecay=3, punch=0.1651, tone=5488.701, toneKeytrack=0.14, drive=6.35, driveMode=1, compressor=0.3135, clipper=0.0324, output=-3.1),
            R("Headroom", "A low-output clean utility preset for heavily processed sessions.", ["utility", "low-output"], voiceMode=0, legato=False, triggerMode=0, velocitySens=58, tune=0, fine=0, bendRange=12, glide=0, body=0.0406, harmonics=0.0346, harmonicBalance=0.2468, pitchDrop=19, pitchDecay=58.6, pitchCurve=0.95, attack=0.4435, hold=0, decay=1068, sustain=0.1864, release=210, ampCurve=0.9756, click=0.3514, clickTone=4952.8302, clickDecay=3, punch=0.3187, tone=6382.9787, toneKeytrack=0.14, drive=8.1, driveMode=1, compressor=0.1397, clipper=0.1857, output=-8.16),
            R("Centered Low", "A solid centered body with restrained transient and medium decay.", ["centered", "restrained"], voiceMode=0, legato=False, triggerMode=0, velocitySens=58, tune=0, fine=0, bendRange=12, glide=0, body=0.0778, harmonics=0.1437, harmonicBalance=0.2719, pitchDrop=24.57, pitchDecay=107.9878, pitchCurve=0.95, attack=0.4965, hold=0, decay=771.1528, sustain=0.0871, release=170, ampCurve=0.6456, click=0.3622, clickTone=5092.0952, clickDecay=3, punch=0.3358, tone=6003.4911, toneKeytrack=0.14, drive=1.1, driveMode=1, compressor=0.3708, clipper=0.0381, output=0.53),
            R("Final Bounce", "A punchy polished final-bounce 808 with predictable peak level.", ["polished", "predictable"], voiceMode=0, legato=False, triggerMode=0, velocitySens=58, tune=0, fine=0, bendRange=12, glide=0, body=0.1149, harmonics=0.1789, harmonicBalance=0.1699, pitchDrop=20.6, pitchDecay=60.3, pitchCurve=0.7, attack=0.2667, hold=0, decay=600, sustain=0.0221, release=105, ampCurve=0.8022, click=0.2558, clickTone=5160.3774, clickDecay=3, punch=0.2215, tone=6723.4043, toneKeytrack=0.14, drive=2.9, driveMode=2, compressor=0.2229, clipper=0.2933, output=-4.93),
        ]),
    ]
)

def unique_tags(values: list[str]) -> list[str]:
    result: list[str] = []
    seen: set[str] = set()
    for value in values:
        key = value.casefold()
        if key not in seen:
            result.append(value)
            seen.add(key)
    return result


def parameter_appropriate_tags(values: list[str], parameters: dict[str, Any]) -> list[str]:
    """Drop categorical tags contradicted by the finished parameter recipe."""
    tags = unique_tags(values)

    if parameters["voiceMode"] != 0:
        tags = [tag for tag in tags if tag not in {"mono", "mono-legato"}]
    if parameters["voiceMode"] != 1:
        tags = [tag for tag in tags if tag not in {"poly", "polyphonic"}]
    if parameters["legato"] is not True:
        tags = [tag for tag in tags if tag not in {"legato", "mono-legato"}]
    if parameters["glide"] <= 0:
        tags = [tag for tag in tags if "glide" not in tag.split("-")]

    return tags


def build_presets() -> list[dict[str, Any]]:
    presets: list[dict[str, Any]] = []

    for category, recipes in RECIPES.items():
        if category not in CATEGORY_BASES or category not in CATEGORY_TAGS:
            raise ValueError(f"Missing category configuration for {category!r}")

        for source in recipes:
            parameters = dict(DEFAULT_PARAMETERS)
            parameters.update(CATEGORY_BASES[category])
            parameters.update(source["overrides"])

            unknown = set(parameters) - set(PARAMETER_RANGES)
            missing = set(PARAMETER_RANGES) - set(parameters)
            if unknown or missing:
                raise ValueError(
                    f"{source['name']}: unknown parameters={sorted(unknown)}, "
                    f"missing parameters={sorted(missing)}"
                )

            # Preserve the declared parameter order for clean diffs and stable binary data.
            ordered_parameters = OrderedDict(
                (parameter_id, parameters[parameter_id])
                for parameter_id in PARAMETER_RANGES
            )

            presets.append(
                OrderedDict(
                    [
                        ("schemaVersion", SCHEMA_VERSION),
                        ("name", source["name"]),
                        ("category", category),
                        ("description", source["description"]),
                        (
                            "tags",
                            parameter_appropriate_tags(
                                CATEGORY_TAGS[category] + source["tags"],
                                parameters,
                            ),
                        ),
                        ("author", AUTHOR),
                        ("parameters", ordered_parameters),
                    ]
                )
            )

    if len(presets) != EXPECTED_COUNT:
        raise ValueError(f"Expected {EXPECTED_COUNT} recipes, found {len(presets)}")

    return presets


def main() -> None:
    presets = build_presets()
    bank = OrderedDict(
        [
            ("schemaVersion", SCHEMA_VERSION),
            ("bankName", "808Glo Pro Factory Bank"),
            ("author", AUTHOR),
            ("presetCount", len(presets)),
            ("presets", presets),
        ]
    )

    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT_PATH.write_text(
        json.dumps(bank, indent=2, ensure_ascii=False) + "\n",
        encoding="utf-8",
    )
    print(f"Generated {len(presets)} presets at {OUTPUT_PATH}")


if __name__ == "__main__":
    main()
