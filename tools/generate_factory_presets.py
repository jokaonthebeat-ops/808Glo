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
            R("Pure Current", "A pure sine-led foundation with a deep, even tail.", ["pure", "long"], body=0.0, harmonics=0.0, pitchDrop=18, pitchDecay=42, decay=2200, tone=900, click=0.02, punch=0.12, output=-5),
            R("Deep Water", "A soft slow-blooming sub for sparse arrangements.", ["soft", "deep"], body=0.03, harmonics=0.01, pitchDrop=10, pitchDecay=72, attack=2.5, decay=3400, sustain=0.06, release=460, tone=620, output=-4),
            R("Night Foundation", "A round clean 808 with a firmer front edge.", ["round", "firm"], body=0.08, harmonics=0.04, pitchDrop=24, pitchDecay=31, pitchCurve=0.72, decay=1500, click=0.08, punch=0.30, tone=1100),
            R("Silent Pressure", "An ultra-dark sustained sub that leaves the upper mix untouched.", ["dark", "sustained"], legato=True, triggerMode=1, glide=120, body=0.02, harmonics=0.0, pitchDrop=7, pitchDecay=95, attack=4, decay=4200, sustain=0.22, release=760, tone=450, click=0, drive=0, compressor=0.03, output=-3.5),
            R("Round Table", "A rounded triangle blend with restrained tape color.", ["rounded", "tape"], body=0.16, harmonics=0.08, harmonicBalance=0.45, pitchDrop=20, pitchDecay=55, decay=1200, tone=1600, drive=2, driveMode=1, click=0.10, punch=0.28, output=-6),
            R("Mono Gravity", "A weighty mono sub with a subtle playable slide.", ["weighty", "slide"], legato=True, triggerMode=1, glide=82, body=0.06, harmonics=0.025, pitchDrop=14, pitchDecay=38, pitchCurve=0.62, decay=2800, sustain=0.08, release=360, tone=760, output=-4),
            R("Clean Sweep", "A clean high-impact drop that settles into an uncluttered sub.", ["impact", "clean"], body=0.11, harmonics=0.05, pitchDrop=31, pitchDecay=24, pitchCurve=0.48, decay=820, release=75, tone=2100, click=0.15, clickTone=5400, punch=0.46, output=-6.5),
            R("Ocean Floor", "A huge slow sub tail designed for open half-time patterns.", ["huge", "half-time"], legato=True, triggerMode=1, glide=155, body=0.0, harmonics=0.0, pitchDrop=5, pitchDecay=130, pitchCurve=1.8, attack=6, decay=5600, sustain=0.28, release=980, tone=350, click=0, punch=0.05, output=-3),
            R("Fundamental One", "A reliable fundamental-first 808 for general production.", ["balanced", "utility"], body=0.09, harmonics=0.04, pitchDrop=16, pitchDecay=48, decay=1800, release=210, tone=1250, drive=1, click=0.05, punch=0.22),
            R("Subtle Giant", "A broad clean body with just enough harmonic translation.", ["broad", "translation"], body=0.18, harmonics=0.10, harmonicBalance=0.25, pitchDrop=11, pitchDecay=78, attack=2.2, decay=2700, sustain=0.10, release=520, tone=1850, drive=1.5, compressor=0.10, output=-5),
            R("Glass Basement", "A polished sub with a light glassy second harmonic.", ["polished", "even-harmonic"], body=0.07, harmonics=0.13, harmonicBalance=0.82, pitchDrop=13, pitchDecay=45, decay=1650, tone=2300, click=0.03, drive=1, output=-5.5),
            R("Low Meridian", "A stable tonal sub with gentle key tracking for melodic basslines.", ["tonal", "melodic"], voiceMode=1, triggerMode=1, velocitySens=42, body=0.04, harmonics=0.055, pitchDrop=4, pitchDecay=20, pitchCurve=0.4, attack=3, decay=2600, sustain=0.55, release=620, tone=1300, toneKeytrack=0.48, click=0, punch=0.08, output=-5),
        ]),
        ("Trap", [
            R("Redline Bounce", "A hard modern trap 808 with a fast redline knock.", ["fast", "knock"], body=0.22, harmonics=0.19, pitchDrop=29, pitchDecay=30, pitchCurve=0.58, decay=900, click=0.38, clickTone=7200, punch=0.70, glide=105, legato=True),
            R("Funeral Trunk", "A dark long trap tail with controlled low-mid weight.", ["dark", "long"], body=0.14, harmonics=0.12, harmonicBalance=-0.1, pitchDrop=24, pitchDecay=58, decay=1900, sustain=0.08, release=320, tone=920, drive=6, driveMode=1, click=0.18, punch=0.42, glide=180, legato=True, output=-5.5),
            R("Crown Pressure", "A clipped crown-heavy hit for aggressive sparse drums.", ["clipped", "aggressive"], body=0.34, harmonics=0.32, pitchDrop=33, pitchDecay=21, pitchCurve=0.43, decay=650, release=65, tone=3900, drive=13, driveMode=3, clipper=0.66, click=0.48, punch=0.82, output=-8.5),
            R("Dark Alley Bend", "A low-passed trap slide with a patient pitch bend.", ["dark", "slide"], legato=True, triggerMode=1, glide=265, body=0.17, harmonics=0.14, pitchDrop=20, pitchDecay=82, pitchCurve=1.55, decay=2200, sustain=0.22, release=440, tone=720, click=0.10, drive=7, output=-6),
            R("Chrome Rattle", "A bright metallic trap body that cuts through small speakers.", ["bright", "metallic"], body=0.40, harmonics=0.39, harmonicBalance=-0.75, pitchDrop=26, pitchDecay=39, decay=760, tone=4700, toneKeytrack=0.24, drive=11, driveMode=3, click=0.55, clickTone=9800, punch=0.76, compressor=0.4, output=-8),
            R("Midnight Flex", "A deep smooth trap 808 made for long held notes.", ["smooth", "held"], legato=True, triggerMode=1, glide=320, body=0.10, harmonics=0.07, pitchDrop=18, pitchDecay=98, pitchCurve=1.7, attack=2, decay=2800, sustain=0.36, release=560, tone=650, drive=4, driveMode=1, click=0.07, punch=0.24, output=-4.5),
            R("Heavy Motion", "A balanced trap workhorse with hard punch and usable glide.", ["workhorse", "punchy"], legato=True, glide=150, body=0.25, harmonics=0.23, pitchDrop=30, pitchDecay=28, pitchCurve=0.52, decay=1120, tone=1850, drive=9, click=0.36, punch=0.72),
            R("Black Diamond", "A polished asymmetrical 808 with expensive low-mid grit.", ["polished", "asymmetrical"], legato=True, triggerMode=1, glide=215, body=0.28, harmonics=0.28, harmonicBalance=0.34, pitchDrop=22, pitchDecay=62, decay=1550, sustain=0.08, release=245, tone=1350, drive=12, driveMode=2, compressor=0.42, clipper=0.50, output=-7.5),
            R("Glo Season", "A sharp short-drop trap hit with an oversized first transient.", ["sharp", "transient"], body=0.20, harmonics=0.17, pitchDrop=38, pitchDecay=17, pitchCurve=0.32, decay=580, release=45, tone=3100, click=0.62, clickTone=11000, clickDecay=1.5, punch=0.92, output=-8.5),
            R("Trunk Ritual", "A slow dirty trunk rumbler with a wide tonal footprint.", ["rumble", "dirty"], legato=True, triggerMode=1, glide=285, body=0.35, harmonics=0.27, harmonicBalance=0.28, pitchDrop=16, pitchDecay=118, pitchCurve=2.0, attack=3, decay=3100, sustain=0.30, release=690, tone=820, drive=9, driveMode=1, click=0.05, punch=0.20, output=-5.5),
            R("Onyx Knock", "A dry centered trap knock with a disciplined tail.", ["dry", "centered"], body=0.18, harmonics=0.20, pitchDrop=28, pitchDecay=25, decay=720, release=70, tone=2400, click=0.42, clickTone=6200, clickDecay=2.0, punch=0.84, drive=7, compressor=0.26, output=-7),
            R("Pressure Point", "A compressed mid-forward trap 808 for dense arrangements.", ["compressed", "mid-forward"], body=0.31, harmonics=0.36, harmonicBalance=-0.62, pitchDrop=21, pitchDecay=47, decay=980, tone=3300, drive=10, driveMode=2, compressor=0.62, clipper=0.38, click=0.28, punch=0.58, output=-8),
            R("Dead Center", "A mono-solid trap fundamental with no wasted upper fizz.", ["mono", "focused"], body=0.08, harmonics=0.09, pitchDrop=25, pitchDecay=36, decay=1300, tone=1050, drive=3, driveMode=1, click=0.16, punch=0.46, compressor=0.20, clipper=0.14, output=-5.5),
            R("Afterdark Weight", "A sustained after-hours trap bass with smooth legato movement.", ["sustained", "legato"], legato=True, triggerMode=1, glide=390, body=0.16, harmonics=0.18, pitchDrop=14, pitchDecay=105, pitchCurve=1.9, attack=2.8, decay=3500, sustain=0.52, release=850, tone=780, drive=5, driveMode=1, click=0.04, punch=0.18, output=-5),
        ]),
        ("Distorted", [
            R("Furnace Teeth", "A hard-clipped furnace 808 with retained fundamental weight.", ["hard-clip", "dense"], body=0.58, harmonics=0.66, pitchDrop=25, pitchDecay=29, decay=820, tone=2900, drive=27, click=0.56, punch=0.76, output=-10.5),
            R("Burn Notice", "A warm asymmetrical burn with a longer musical tail.", ["warm", "asymmetrical"], legato=True, glide=105, body=0.43, harmonics=0.52, harmonicBalance=0.20, pitchDrop=18, pitchDecay=62, decay=1450, sustain=0.07, release=190, tone=1550, drive=22, driveMode=2, compressor=0.52, click=0.34, output=-9),
            R("Crushed Chrome", "A bright crushed 808 for maximal high-frequency presence.", ["bright", "crushed"], body=0.72, harmonics=0.82, harmonicBalance=-0.78, pitchDrop=31, pitchDecay=19, pitchCurve=0.38, decay=520, release=55, tone=6400, drive=33, click=0.77, clickTone=12400, clickDecay=1.2, punch=0.90, clipper=0.92, output=-12),
            R("Rabid Circuit", "A folding circuit-bass with animated odd-harmonic bite.", ["folded", "odd-harmonic"], legato=True, triggerMode=1, glide=245, body=0.84, harmonics=0.94, harmonicBalance=-0.90, pitchDrop=12, pitchDecay=98, pitchCurve=1.8, decay=1150, sustain=0.12, release=290, tone=3900, drive=35, click=0.42, punch=0.54, output=-12),
            R("Rusted Crown", "A tube-scorched 808 with a thick controlled midrange.", ["tube", "thick"], legato=True, glide=150, body=0.48, harmonics=0.45, harmonicBalance=0.18, pitchDrop=20, pitchDecay=46, decay=1850, sustain=0.10, release=360, tone=1120, drive=19, driveMode=1, compressor=0.56, clipper=0.44, click=0.26, output=-8.5),
            R("Asphalt Grinder", "A slow grinding 808 with resonant low-mid distortion.", ["resonant", "slow"], legato=True, triggerMode=1, glide=305, body=0.64, harmonics=0.73, pitchDrop=8, pitchDecay=115, pitchCurve=2.2, attack=4, decay=2550, sustain=0.24, release=620, tone=710, drive=29, click=0.12, punch=0.24, output=-11),
            R("Voltage Scar", "A short voltage spike with a clean sub beneath the damage.", ["short", "layered"], body=0.36, harmonics=0.42, pitchDrop=29, pitchDecay=24, pitchCurve=0.44, decay=650, release=62, tone=1950, drive=21, click=0.70, clickTone=9200, punch=0.88, compressor=0.34, output=-9.5),
            R("Broken Amp", "A blown-amplifier texture with an uneven aggressive edge.", ["blown-amp", "rough"], legato=True, triggerMode=1, glide=182, body=0.77, harmonics=0.86, harmonicBalance=0.62, pitchDrop=16, pitchDecay=78, decay=1320, sustain=0.09, release=230, tone=980, drive=31, driveMode=2, compressor=0.46, click=0.20, output=-11.5),
            R("Red Static", "An extreme short static hit for rage and industrial patterns.", ["extreme", "rage"], body=0.92, harmonics=1.0, harmonicBalance=-1.0, pitchDrop=40, pitchDecay=14, pitchCurve=0.27, decay=380, release=35, tone=7200, drive=36, click=0.92, clickTone=14500, clickDecay=0.8, punch=1.0, compressor=0.68, clipper=1.0, output=-13),
            R("Iron Lung", "A dark breathing distortion with a very long gated tail.", ["dark", "gated"], legato=True, triggerMode=1, glide=405, body=0.33, harmonics=0.56, harmonicBalance=0.40, pitchDrop=10, pitchDecay=145, pitchCurve=2.4, attack=6, decay=3600, sustain=0.42, release=920, tone=560, drive=24, driveMode=1, click=0.04, punch=0.14, output=-9),
            R("Concrete Shred", "A midrange shredder tuned for audible laptop-speaker grit.", ["midrange", "speaker-ready"], body=0.68, harmonics=0.78, harmonicBalance=-0.30, pitchDrop=22, pitchDecay=38, decay=900, tone=4400, drive=30, click=0.50, clickTone=8000, punch=0.72, compressor=0.58, clipper=0.82, output=-11),
            R("Circuit Bruise", "A bruised soft-clip tone with heavy even-harmonic bloom.", ["soft-clip", "even-harmonic"], body=0.52, harmonics=0.75, harmonicBalance=0.88, pitchDrop=15, pitchDecay=70, decay=1750, sustain=0.14, release=330, tone=2300, drive=26, driveMode=2, click=0.22, punch=0.48, output=-10),
            R("Speaker Torch", "A fiercely bright clipped 808 designed for controlled destruction.", ["bright", "clipped"], body=0.80, harmonics=0.90, harmonicBalance=-0.65, pitchDrop=34, pitchDecay=21, decay=700, tone=9200, toneKeytrack=0.35, drive=34, click=0.82, clickTone=13200, clickDecay=1.1, punch=0.95, compressor=0.65, clipper=0.96, output=-12.5),
        ]),
        ("Drill", [
            R("Cold Step", "A dark controlled drill 808 with a precise medium slide.", ["dark", "controlled"], glide=180, body=0.19, harmonics=0.18, pitchDrop=22, pitchDecay=56, decay=1250, sustain=0.12, release=190, tone=1000, drive=8, click=0.22, punch=0.48),
            R("Sliding Shadow", "A deep long drill slide that moves without losing pitch center.", ["deep", "long-slide"], glide=425, body=0.14, harmonics=0.11, pitchDrop=18, pitchDecay=82, pitchCurve=1.6, attack=1.2, decay=1900, sustain=0.28, release=380, tone=720, drive=6, driveMode=1, click=0.10, output=-5),
            R("Frozen Block", "A hard icy drill hit with a fast defined glide.", ["hard", "icy"], glide=138, body=0.30, harmonics=0.33, pitchDrop=31, pitchDecay=34, pitchCurve=0.66, decay=760, sustain=0.04, release=95, tone=1950, drive=12, driveMode=3, click=0.48, clickTone=8200, punch=0.78, output=-8),
            R("Concrete Freeze", "An ultra-low sustained drill foundation for sparse piano loops.", ["sustained", "ultra-low"], glide=365, body=0.11, harmonics=0.07, pitchDrop=14, pitchDecay=102, pitchCurve=1.9, attack=2.2, decay=2500, sustain=0.42, release=520, tone=540, drive=4, driveMode=1, click=0.04, punch=0.16, output=-4),
            R("Dark March", "A gritty marching drill bass with an assertive upper edge.", ["gritty", "mid-forward"], glide=255, body=0.36, harmonics=0.40, harmonicBalance=-0.72, pitchDrop=24, pitchDecay=64, decay=980, sustain=0.10, release=155, tone=1280, drive=14, driveMode=2, click=0.33, punch=0.61, output=-8),
            R("Grit Slide", "A saturated drill slide with audible movement on small speakers.", ["saturated", "speaker-ready"], glide=325, body=0.42, harmonics=0.46, harmonicBalance=-0.50, pitchDrop=28, pitchDecay=45, decay=1450, sustain=0.20, release=270, tone=1650, drive=15, driveMode=3, click=0.54, punch=0.76, compressor=0.45, output=-9),
            R("Tension Wire", "A resonant slow-bending 808 that builds suspense between notes.", ["resonant", "slow-bend"], glide=505, body=0.23, harmonics=0.26, harmonicBalance=0.42, pitchDrop=12, pitchDecay=128, pitchCurve=2.3, attack=3, decay=2200, sustain=0.38, release=720, tone=680, drive=9, click=0.07, punch=0.22, output=-6.5),
            R("Alley Phantom", "A nearly pure phantom sub with an extra-long legato tail.", ["pure", "phantom"], glide=610, body=0.05, harmonics=0.025, pitchDrop=20, pitchDecay=72, attack=4, decay=3100, sustain=0.58, release=840, tone=450, drive=2, driveMode=1, click=0, punch=0.08, compressor=0.12, clipper=0.08, output=-3.5),
            R("Black Puffer", "A compact hard drill knock with rapid pitch settling.", ["compact", "knock"], glide=92, body=0.32, harmonics=0.36, pitchDrop=27, pitchDecay=27, pitchCurve=0.42, decay=620, sustain=0, release=68, tone=2350, drive=11, click=0.64, clickTone=9400, punch=0.86, output=-8.5),
            R("Winter Pressure", "A balanced winter-dark slide suitable for full drill arrangements.", ["balanced", "dark"], glide=282, body=0.17, harmonics=0.20, pitchDrop=16, pitchDecay=92, pitchCurve=1.75, decay=1650, sustain=0.22, release=330, tone=820, drive=7, click=0.16, punch=0.38, output=-6),
            R("Steel Slide", "A bright steel-edged drill glide for fast octave movement.", ["bright", "octave-slide"], glide=205, body=0.38, harmonics=0.44, harmonicBalance=-0.62, pitchDrop=25, pitchDecay=48, decay=1080, sustain=0.15, release=180, tone=2800, toneKeytrack=0.38, drive=13, click=0.40, punch=0.67, output=-8),
            R("North Wind", "A smooth cold glide with softened transients and a long release.", ["smooth", "soft-transient"], glide=470, body=0.12, harmonics=0.09, pitchDrop=9, pitchDecay=116, pitchCurve=2.0, attack=5, decay=2700, sustain=0.48, release=950, tone=610, drive=3, click=0.02, punch=0.12, output=-4.5),
            R("Ghost Route", "A nimble ghost-note drill 808 with a short controllable slide.", ["nimble", "ghost-note"], glide=115, body=0.21, harmonics=0.24, pitchDrop=19, pitchDecay=40, decay=720, sustain=0.08, release=110, tone=1450, drive=8, click=0.26, clickTone=5600, punch=0.52, output=-6.5),
        ]),
        ("Detroit", [
            R("Motor Bounce", "A short motor-city bounce with a crisp front edge.", ["crisp", "classic"], body=0.30, harmonics=0.35, pitchDrop=25, pitchDecay=26, decay=340, click=0.66, clickTone=8200, punch=0.86),
            R("Offbeat Money", "A slightly rounder short 808 built for syncopated pockets.", ["syncopated", "round"], body=0.24, harmonics=0.27, pitchDrop=18, pitchDecay=36, decay=470, release=72, tone=2150, drive=7, click=0.50, punch=0.70, output=-7),
            R("Buffed Up", "An extremely tight bright knock with hard clipping.", ["tight", "bright"], body=0.39, harmonics=0.48, pitchDrop=29, pitchDecay=17, pitchCurve=0.34, decay=260, release=34, tone=5200, drive=15, driveMode=3, click=0.82, clickTone=11800, clickDecay=1.0, punch=0.96, clipper=0.72, output=-10),
            R("Woodward Knock", "A woody centered knock with extra fundamental beneath it.", ["woody", "centered"], body=0.17, harmonics=0.18, harmonicBalance=0.60, pitchDrop=20, pitchDecay=43, decay=590, release=105, tone=1620, drive=6, driveMode=1, click=0.44, clickTone=4200, clickDecay=3.5, punch=0.66, output=-6),
            R("Motor City Glide", "A rare sliding Detroit 808 that stays short and rhythmic.", ["slide", "rhythmic"], legato=True, triggerMode=1, glide=92, body=0.28, harmonics=0.31, pitchDrop=14, pitchDecay=61, decay=700, sustain=0.08, release=145, tone=1200, drive=8, driveMode=1, click=0.36, punch=0.58, output=-7),
            R("Punchline Pocket", "A one-shot punctuation hit with maximum initial punch.", ["one-shot", "maximum-punch"], body=0.35, harmonics=0.42, pitchDrop=34, pitchDecay=14, pitchCurve=0.28, decay=180, release=20, tone=6800, drive=13, driveMode=2, click=0.94, clickTone=13800, clickDecay=0.7, punch=1.0, output=-11),
            R("Fast Talk", "A rapid clean-cut 808 for dense talk-over-the-beat patterns.", ["rapid", "dense-pattern"], body=0.22, harmonics=0.32, pitchDrop=27, pitchDecay=21, decay=300, release=42, tone=3700, drive=10, click=0.74, clickTone=9600, clickDecay=1.3, punch=0.90, output=-9),
            R("Coney Bass", "A dark rounded Detroit bass with a little more tail.", ["dark", "rounded"], body=0.11, harmonics=0.10, pitchDrop=10, pitchDecay=76, pitchCurve=1.5, attack=2, decay=660, sustain=0.04, release=150, tone=800, drive=4, driveMode=1, click=0.22, punch=0.40, output=-5),
            R("Paper Route", "A mid-heavy clipped bounce that survives phone playback.", ["mid-heavy", "phone-ready"], body=0.43, harmonics=0.56, harmonicBalance=-0.75, pitchDrop=16, pitchDecay=52, decay=410, release=82, tone=2700, drive=16, click=0.60, punch=0.78, compressor=0.52, output=-10),
            R("Side Door", "A stealthy dry short bass with restrained click and grit.", ["dry", "restrained"], body=0.27, harmonics=0.30, pitchDrop=22, pitchDecay=31, decay=530, release=92, tone=1900, drive=11, driveMode=1, click=0.32, punch=0.64, output=-8),
            R("Quick Cash", "A high-velocity short hit made for fast piano-led beats.", ["high-velocity", "piano-beat"], velocitySens=88, body=0.26, harmonics=0.29, pitchDrop=31, pitchDecay=19, pitchCurve=0.4, decay=230, release=28, tone=4100, drive=9, click=0.78, clickTone=10400, punch=0.95, output=-9.5),
            R("Beltline Kick", "A kick-like 808 hybrid with a compact sub finish.", ["kick-hybrid", "compact"], body=0.18, harmonics=0.22, pitchDrop=42, pitchDecay=24, pitchCurve=0.5, hold=8, decay=320, release=35, tone=3000, drive=8, click=0.86, clickTone=7000, clickDecay=2.2, punch=0.98, output=-9),
        ]),
        ("West Coast", [
            R("Palm Bounce", "A warm round coastal 808 with an easy medium glide.", ["round", "easy-glide"], glide=122, body=0.18, harmonics=0.20, pitchDrop=12, pitchDecay=66, decay=1650, sustain=0.20, release=390, tone=1800, drive=5, click=0.16, punch=0.35),
            R("Chrome Coast", "A polished upper-harmonic coast bass with chrome definition.", ["polished", "defined"], glide=82, body=0.31, harmonics=0.34, harmonicBalance=0.55, pitchDrop=18, pitchDecay=49, decay=1120, sustain=0.08, release=230, tone=3300, drive=8, click=0.34, punch=0.58, output=-7),
            R("Lowrider Lean", "A slow leaning long-tail 808 for spacious west-coast drums.", ["slow", "long-tail"], triggerMode=1, glide=345, body=0.11, harmonics=0.09, pitchDrop=8, pitchDecay=115, pitchCurve=2.0, attack=3, decay=2900, sustain=0.46, release=820, tone=680, drive=4, click=0.04, punch=0.15, output=-4),
            R("Sunset Trunk", "A soft sunset trunk tone with a smooth musical release.", ["soft", "musical"], triggerMode=1, glide=265, body=0.22, harmonics=0.24, pitchDrop=14, pitchDecay=87, pitchCurve=1.55, attack=2, decay=2250, sustain=0.32, release=620, tone=1100, drive=6, click=0.10, punch=0.26, output=-5.5),
            R("G-Funk Floor", "A harmonically rich tonal 808 suited to melodic funk basslines.", ["tonal", "funk"], triggerMode=1, glide=182, body=0.36, harmonics=0.43, harmonicBalance=0.72, pitchDrop=10, pitchDecay=72, decay=1850, sustain=0.26, release=470, tone=4600, toneKeytrack=0.48, drive=9, click=0.22, punch=0.44, output=-7.5),
            R("Hydraulic Drop", "A pronounced downward drop with a springy hydraulic tail.", ["drop", "springy"], glide=142, body=0.26, harmonics=0.31, pitchDrop=25, pitchDecay=41, pitchCurve=0.72, decay=920, sustain=0.05, release=170, tone=2450, drive=10, driveMode=2, click=0.46, punch=0.72, output=-8),
            R("Boulevard Bass", "A pure boulevard sub that glides beneath busy instrumentation.", ["pure", "under-mix"], triggerMode=1, glide=425, body=0.07, harmonics=0.035, pitchDrop=6, pitchDecay=135, pitchCurve=2.3, attack=5, decay=3500, sustain=0.60, release=980, tone=500, drive=2, click=0, punch=0.08, output=-3.5),
            R("Coastline Smoke", "A dark smoky legato tone with a soft tube edge.", ["smoky", "tube"], triggerMode=1, glide=305, body=0.15, harmonics=0.16, pitchDrop=16, pitchDecay=98, attack=4, decay=3100, sustain=0.50, release=1050, tone=760, drive=7, click=0.06, punch=0.18, output=-5),
            R("Coast Chrome", "A bright modern coast 808 with wide upper-harmonic character.", ["bright", "modern"], glide=112, body=0.41, harmonics=0.49, harmonicBalance=0.58, pitchDrop=20, pitchDecay=56, decay=1450, sustain=0.12, release=330, tone=5300, toneKeytrack=0.40, drive=11, driveMode=2, click=0.42, punch=0.62, compressor=0.35, output=-9),
            R("Sunday Cruise", "A relaxed cruising 808 with a gentle pitch envelope.", ["relaxed", "gentle"], triggerMode=1, glide=225, body=0.23, harmonics=0.22, pitchDrop=9, pitchDecay=124, pitchCurve=2.1, attack=3, decay=2700, sustain=0.40, release=780, tone=1500, drive=5, click=0.08, punch=0.22, output=-5),
            R("Daylight Rider", "A clean daylight bass with extra melodic key tracking.", ["clean", "melodic"], voiceMode=1, triggerMode=1, legato=False, glide=0, body=0.14, harmonics=0.18, harmonicBalance=0.68, pitchDrop=5, pitchDecay=28, attack=2.5, decay=2100, sustain=0.58, release=540, tone=2800, toneKeytrack=0.62, drive=3, click=0.03, punch=0.14, output=-5.5),
            R("Golden State Knock", "A firm west-coast knock with warm saturation and short glide.", ["firm", "warm"], glide=68, body=0.28, harmonics=0.30, pitchDrop=22, pitchDecay=37, decay=780, sustain=0.03, release=130, tone=2200, drive=8, driveMode=1, click=0.38, clickTone=5600, punch=0.70, output=-7),
        ]),
        ("Long Glide", [
            R("Endless Bend", "An extremely long clean bend with an almost continuous tail.", ["extreme-glide", "clean"], glide=720, body=0.09, harmonics=0.07, pitchDrop=10, pitchDecay=82, decay=4700, sustain=0.86, release=1250, tone=720, drive=2, click=0.03, punch=0.12, output=-4),
            R("Silk Slide", "A smooth silky legato slide with soft even harmonics.", ["smooth", "even-harmonic"], glide=525, body=0.17, harmonics=0.18, harmonicBalance=0.72, pitchDrop=14, pitchDecay=62, decay=3900, sustain=0.72, release=980, tone=1100, drive=5, click=0.10, punch=0.22, output=-5),
            R("Lunar Portamento", "A wide tonal portamento preset for dramatic octave jumps.", ["portamento", "octave"], glide=920, body=0.31, harmonics=0.43, harmonicBalance=-0.18, pitchDrop=6, pitchDecay=145, pitchCurve=2.4, attack=5, decay=6000, sustain=0.95, release=1500, tone=2450, toneKeytrack=0.55, drive=8, driveMode=2, click=0, punch=0.10, output=-8),
            R("Slow Lane", "A pure slow-lane sub with understated note transitions.", ["pure", "understated"], glide=625, body=0.04, harmonics=0.01, pitchDrop=7, pitchDecay=112, pitchCurve=1.9, attack=4, decay=4300, sustain=0.90, release=1150, tone=480, drive=0.5, click=0, punch=0.06, output=-3),
            R("Deep Curve", "A dark curved glide with enough harmonics to remain audible.", ["dark", "audible"], glide=465, body=0.24, harmonics=0.26, pitchDrop=18, pitchDecay=92, decay=3600, sustain=0.66, release=820, tone=860, drive=7, click=0.13, punch=0.30, output=-6),
            R("Serpent Tail", "A winding saturated tail with pronounced odd-harmonic motion.", ["winding", "odd-harmonic"], glide=785, body=0.39, harmonics=0.56, harmonicBalance=-0.82, pitchDrop=12, pitchDecay=165, pitchCurve=2.6, attack=3, decay=4900, sustain=0.80, release=1080, tone=1750, drive=13, driveMode=3, click=0.06, punch=0.18, output=-9),
            R("Ghost Glide", "A nearly invisible fundamental-only glide for layering.", ["fundamental", "layering"], glide=860, body=0.01, harmonics=0, pitchDrop=0, pitchDecay=8, pitchCurve=1, attack=8, decay=6500, sustain=1.0, release=1600, tone=350, drive=0, compressor=0.04, clipper=0, click=0, punch=0, output=-2.5),
            R("Late Arrival", "A delayed-feeling pitch fall followed by a medium-long glide.", ["delayed-pitch", "medium-glide"], glide=385, body=0.20, harmonics=0.23, pitchDrop=20, pitchDecay=72, pitchCurve=1.8, decay=3300, sustain=0.62, release=740, tone=1320, drive=6, click=0.18, punch=0.38, output=-5.5),
            R("Infinite Trunk", "A massive sustained trunk note with reliable mono legato behavior.", ["massive", "trunk"], glide=645, body=0.13, harmonics=0.11, pitchDrop=9, pitchDecay=125, pitchCurve=2.2, attack=6, decay=7200, sustain=0.96, release=1800, tone=620, drive=4, click=0.02, punch=0.10, output=-4),
            R("Afterhours Drift", "A harmonically rich after-hours drift with a relaxed release.", ["rich", "relaxed"], glide=565, body=0.32, harmonics=0.39, harmonicBalance=0.30, pitchDrop=15, pitchDecay=102, decay=4700, sustain=0.74, release=1100, tone=2150, drive=10, driveMode=2, click=0.08, punch=0.24, output=-7.5),
            R("Long Game", "A practical long glide tuned for sustained modern bass melodies.", ["practical", "bass-melody"], glide=440, body=0.16, harmonics=0.17, pitchDrop=8, pitchDecay=68, decay=4000, sustain=0.78, release=920, tone=1450, toneKeytrack=0.42, drive=4, click=0.05, punch=0.16, output=-5),
            R("Gravity Lane", "A heavy downward glide with a pronounced opening pitch drop.", ["heavy", "downward"], glide=690, body=0.26, harmonics=0.28, pitchDrop=28, pitchDecay=115, pitchCurve=1.65, attack=1.2, decay=5200, sustain=0.82, release=1300, tone=980, drive=8, click=0.14, punch=0.42, output=-6.5),
        ]),
        ("Short Punch", [
            R("Kickback", "A compact kickback hit with a hard descending transient.", ["compact", "hard"], body=0.28, harmonics=0.35, pitchDrop=32, pitchDecay=17, pitchCurve=0.42, decay=180, release=24, tone=4900, drive=10, click=0.90, clickTone=9600, clickDecay=1.0, punch=0.98, output=-10),
            R("Chest Tap", "A round chest-level thump with a slightly softer click.", ["round", "thump"], body=0.16, harmonics=0.18, pitchDrop=24, pitchDecay=28, decay=285, release=45, tone=2600, drive=6, driveMode=1, click=0.68, clickTone=5200, clickDecay=2.8, punch=0.74, output=-8),
            R("Quick Knock", "An ultra-short high-impact knock for rapid patterns.", ["ultra-short", "rapid"], body=0.22, harmonics=0.30, pitchDrop=38, pitchDecay=11, pitchCurve=0.27, hold=2, decay=110, release=12, tone=7400, drive=13, driveMode=3, click=1.0, clickTone=14500, clickDecay=0.55, punch=1.0, clipper=0.70, output=-12),
            R("Tight Pocket", "A disciplined tight 808 that stays clear around fast kicks.", ["disciplined", "clear"], body=0.10, harmonics=0.10, pitchDrop=18, pitchDecay=36, decay=365, release=62, tone=1500, drive=4, driveMode=1, click=0.52, clickTone=4400, punch=0.66, output=-6),
            R("Staccato Sub", "A pure staccato sub with almost no upper harmonic content.", ["pure", "staccato"], body=0.0, harmonics=0.0, pitchDrop=12, pitchDecay=46, pitchCurve=0.9, attack=1, hold=0, decay=220, release=38, tone=700, drive=0, compressor=0.08, clipper=0.04, click=0.25, punch=0.44, output=-4.5),
            R("One Two", "A two-part punch with bright click followed by a thick body.", ["two-part", "bright"], body=0.35, harmonics=0.43, pitchDrop=29, pitchDecay=21, decay=425, release=78, tone=3500, drive=11, driveMode=2, click=0.80, clickTone=11000, clickDecay=2.5, punch=0.85, output=-9),
            R("Short Fuse", "An explosive clipped micro-tail for rage-style bass rhythms.", ["explosive", "rage"], body=0.46, harmonics=0.61, pitchDrop=34, pitchDecay=14, pitchCurve=0.31, hold=1, decay=85, release=10, tone=9200, drive=17, driveMode=3, click=0.96, clickTone=15500, clickDecay=0.45, punch=1.0, compressor=0.48, clipper=0.82, output=-12),
            R("Snap Weight", "A snapping attack paired with a medium-short weighty tail.", ["snap", "weighty"], body=0.18, harmonics=0.22, pitchDrop=20, pitchDecay=31, decay=510, release=92, tone=2050, drive=7, click=0.62, clickTone=7600, punch=0.72, output=-7),
            R("Pocket Hammer", "A dense hammer strike that remains centered and mono-safe.", ["dense", "mono-safe"], body=0.31, harmonics=0.39, pitchDrop=27, pitchDecay=24, decay=325, release=48, tone=4300, drive=14, driveMode=2, click=0.86, clickTone=8900, clickDecay=1.4, punch=0.94, compressor=0.40, output=-10),
            R("Dry Impact", "A dry woody impact with minimal saturation and no lingering tail.", ["dry", "woody"], body=0.11, harmonics=0.08, harmonicBalance=0.65, pitchDrop=15, pitchDecay=39, decay=145, release=20, tone=1100, drive=2, driveMode=0, click=0.72, clickTone=3200, clickDecay=3.8, punch=0.78, compressor=0.16, clipper=0.10, output=-6),
            R("Fast Cut", "A high-passed-feeling short cut that leaves maximum bass headroom.", ["fast", "headroom"], body=0.20, harmonics=0.27, pitchDrop=26, pitchDecay=19, decay=200, release=28, tone=3300, drive=6, click=0.76, clickTone=12200, clickDecay=0.9, punch=0.88, output=-8),
            R("Mini Monster", "A tiny tail with oversized harmonics and hard clip character.", ["tiny-tail", "oversized"], body=0.40, harmonics=0.52, harmonicBalance=-0.70, pitchDrop=30, pitchDecay=16, decay=130, release=16, tone=5600, drive=16, driveMode=3, click=0.88, clickTone=10000, clickDecay=1.1, punch=0.96, clipper=0.76, output=-11),
        ]),
        ("Experimental", [
            R("Zero Gravity", "A floating polyphonic bass texture with slow pitch movement.", ["polyphonic", "floating"], voiceMode=1, legato=False, triggerMode=1, glide=0, fine=-5, body=0.62, harmonics=0.80, harmonicBalance=0.35, pitchDrop=4, pitchDecay=240, pitchCurve=3.2, attack=9, decay=2700, sustain=0.48, release=760, tone=2900, drive=20, click=0, output=-11),
            R("Phase Beast", "A high-resonance harmonic beast with extreme portamento.", ["resonant", "extreme"], legato=True, triggerMode=1, glide=980, body=0.86, harmonics=0.96, harmonicBalance=-0.92, pitchDrop=31, pitchDecay=185, pitchCurve=3.6, decay=1900, sustain=0.25, release=470, tone=5400, toneKeytrack=0.70, drive=29, click=0.34, output=-12),
            R("Bent Glass", "A glassy poly bass with detuning and a slow exaggerated pitch fall.", ["glassy", "detuned"], voiceMode=1, legato=False, triggerMode=1, glide=0, fine=12, body=0.73, harmonics=0.89, harmonicBalance=0.88, pitchDrop=46, pitchDecay=360, pitchCurve=3.8, attack=16, hold=35, decay=3500, sustain=0.62, release=980, tone=9200, drive=18, driveMode=2, click=0.70, clickTone=14000, clickDecay=11, output=-12),
            R("Rubber Room", "A rubbery slow-attack bass that bends dramatically between notes.", ["rubbery", "slow-attack"], legato=True, triggerMode=1, glide=835, body=0.49, harmonics=0.72, harmonicBalance=0.48, pitchDrop=7, pitchDecay=310, pitchCurve=3.4, attack=22, hold=45, decay=2300, sustain=0.36, release=820, tone=1250, drive=24, click=0.08, punch=0.18, output=-10),
            R("Alien Trunk", "An extreme alien one-shot with folding highs and maximum pitch drop.", ["alien", "one-shot"], legato=False, triggerMode=0, glide=0, body=0.96, harmonics=1.0, harmonicBalance=-0.85, pitchDrop=48, pitchDecay=480, pitchCurve=4.0, attack=0.05, decay=920, sustain=0, release=210, tone=14500, drive=36, click=0.94, clickTone=16000, clickDecay=18, punch=1.0, compressor=0.72, clipper=1.0, output=-14),
            R("Octave Ghost", "A ghostly sub transposed down an octave with a huge sustained tail.", ["octave-down", "ghostly"], legato=True, triggerMode=1, tune=-12, fine=-3, glide=1100, body=0.39, harmonics=0.61, pitchDrop=12, pitchDecay=260, pitchCurve=3.0, attack=13, decay=6200, sustain=0.84, release=1500, tone=480, drive=14, driveMode=1, click=0, punch=0.10, output=-8),
            R("Circuit Melt", "A melting circuit tone with dense odd harmonics and long glide.", ["melting", "odd-harmonic"], legato=True, triggerMode=1, glide=620, body=0.79, harmonics=1.0, harmonicBalance=-1.0, pitchDrop=25, pitchDecay=135, pitchCurve=2.4, attack=1, decay=1550, sustain=0.18, release=370, tone=3700, drive=33, click=0.55, clickTone=11800, clickDecay=7, punch=0.64, output=-12),
            R("Hollow Square", "A hollow square-like bass with even harmonics and gate control.", ["hollow", "square-like"], legato=True, triggerMode=1, glide=430, body=1.0, harmonics=0.91, harmonicBalance=1.0, pitchDrop=8, pitchDecay=190, pitchCurve=2.7, attack=5, decay=2900, sustain=0.54, release=930, tone=760, drive=22, driveMode=2, click=0.18, punch=0.34, output=-10),
            R("Radioactive Sub", "A radioactive resonant sub with slow attack and unstable color.", ["radioactive", "resonant"], legato=True, triggerMode=1, glide=760, body=0.53, harmonics=0.77, harmonicBalance=-0.12, pitchDrop=3, pitchDecay=290, pitchCurve=3.5, attack=28, decay=4300, sustain=0.72, release=1180, tone=420, toneKeytrack=0.82, drive=31, click=0.04, punch=0.12, output=-11),
            R("Reverse Gravity", "A slow-settling pitch illusion with maximal expressive glide.", ["pitch-illusion", "expressive"], legato=True, triggerMode=1, glide=1250, body=0.68, harmonics=0.93, harmonicBalance=0.22, pitchDrop=2, pitchDecay=420, pitchCurve=4.0, attack=19, hold=80, decay=3700, sustain=0.68, release=1050, tone=6500, drive=27, driveMode=2, click=0.44, clickDecay=14, punch=0.30, output=-12),
            R("Broken Orbit", "A polyphonic octave-up effect bass with a fractured short envelope.", ["octave-up", "fractured"], voiceMode=1, legato=False, triggerMode=0, tune=12, fine=7, glide=0, body=0.82, harmonics=0.90, harmonicBalance=-0.66, pitchDrop=39, pitchDecay=75, pitchCurve=0.35, attack=0.08, hold=12, decay=680, sustain=0, release=90, tone=7800, drive=30, click=0.82, clickTone=15000, clickDecay=2.6, punch=0.92, output=-13),
            R("Laser Basement", "A laser-like high click over a dark sustained basement sub.", ["laser", "dark-sub"], legato=True, triggerMode=1, glide=350, body=0.12, harmonics=0.34, harmonicBalance=0.70, pitchDrop=44, pitchDecay=210, pitchCurve=2.8, attack=2, decay=5200, sustain=0.78, release=1350, click=1.0, clickTone=15800, clickDecay=22, punch=0.56, tone=560, drive=17, driveMode=1, output=-10),
        ]),
        ("Mix Ready", [
            R("Vocal Pocket", "A controlled dark 808 shaped to leave space for lead vocals.", ["vocal-space", "dark"], body=0.11, harmonics=0.10, pitchDrop=16, pitchDecay=46, decay=1100, release=180, tone=950, drive=3, click=0.14, punch=0.36, compressor=0.28, output=-5),
            R("Small Speaker Safe", "A harmonic-balanced 808 that remains audible on small speakers.", ["small-speaker", "audible"], body=0.25, harmonics=0.30, harmonicBalance=0.28, pitchDrop=20, pitchDecay=35, decay=820, release=120, tone=2850, drive=6, driveMode=2, click=0.30, punch=0.52, compressor=0.42, output=-6.5),
            R("Master Bus Safe", "A conservative clean sub with ample mastering headroom.", ["headroom", "clean"], body=0.07, harmonics=0.04, pitchDrop=12, pitchDecay=61, attack=2, decay=1550, sustain=0.06, release=250, tone=720, drive=1, driveMode=0, click=0.06, punch=0.20, compressor=0.14, clipper=0.06, output=-4.5),
            R("Club Translation", "A club-balanced 808 with firm fundamental and audible mids.", ["club", "balanced"], legato=True, glide=72, body=0.20, harmonics=0.19, pitchDrop=18, pitchDecay=42, decay=1320, sustain=0.06, release=205, tone=1850, drive=5, click=0.26, punch=0.50, compressor=0.38, output=-6),
            R("Streaming Low", "A compact level-controlled low end prepared for streaming masters.", ["streaming", "level-controlled"], body=0.16, harmonics=0.15, pitchDrop=14, pitchDecay=56, attack=1.5, decay=920, release=145, tone=1200, drive=4, driveMode=2, click=0.18, punch=0.40, compressor=0.46, clipper=0.20, output=-5.5),
            R("Tight Master", "A tight clipped body that stays disciplined under bus limiting.", ["tight", "limiter-safe"], body=0.22, harmonics=0.24, pitchDrop=22, pitchDecay=30, pitchCurve=0.66, decay=630, release=88, tone=2350, drive=7, driveMode=3, click=0.36, punch=0.62, compressor=0.36, clipper=0.38, output=-7),
            R("Clean Loud", "A clean loudness-optimized 808 without excessive saturation.", ["loud", "clean"], body=0.13, harmonics=0.12, pitchDrop=18, pitchDecay=39, decay=1020, release=165, tone=1550, drive=3.5, driveMode=2, click=0.24, punch=0.48, compressor=0.52, clipper=0.26, output=-5.5),
            R("Sidechain Space", "A shorter release 808 designed to recover cleanly after ducking.", ["sidechain", "short-release"], legato=True, glide=122, body=0.18, harmonics=0.16, pitchDrop=10, pitchDecay=72, attack=3, decay=710, release=95, tone=1000, drive=3, click=0.10, punch=0.30, compressor=0.25, output=-5),
            R("Mono Certified", "A pure centered low end with minimal phase-sensitive harmonics.", ["mono", "phase-safe"], body=0.08, harmonics=0.055, pitchDrop=15, pitchDecay=51, decay=1220, sustain=0.04, release=225, tone=820, drive=2, driveMode=1, click=0.08, punch=0.28, compressor=0.18, clipper=0.10, output=-4),
            R("Ready Print", "A finished assertive 808 requiring minimal additional processing.", ["finished", "assertive"], legato=True, glide=62, body=0.26, harmonics=0.31, pitchDrop=24, pitchDecay=28, pitchCurve=0.58, decay=570, release=78, tone=3250, drive=8, driveMode=2, click=0.44, punch=0.70, compressor=0.50, clipper=0.44, output=-7.5),
            R("Vocal Gap", "A scooped low-mid 808 with smooth sustain beneath dense vocals.", ["scooped", "vocal-space"], legato=True, triggerMode=1, glide=155, body=0.10, harmonics=0.08, pitchDrop=13, pitchDecay=70, attack=2.5, decay=1800, sustain=0.22, release=360, tone=680, drive=2.5, click=0.07, punch=0.24, compressor=0.22, output=-4.5),
            R("Phone Weight", "A carefully driven 808 with second harmonics that survive phone playback.", ["phone", "second-harmonic"], body=0.23, harmonics=0.34, harmonicBalance=0.85, pitchDrop=17, pitchDecay=41, decay=900, release=130, tone=3600, toneKeytrack=0.30, drive=7, driveMode=1, click=0.22, punch=0.44, compressor=0.45, clipper=0.30, output=-7),
            R("Car Check", "A car-system-focused 808 with controlled long-bass resonance.", ["car", "long"], legato=True, glide=95, body=0.17, harmonics=0.14, pitchDrop=19, pitchDecay=54, decay=1650, sustain=0.12, release=290, tone=1350, drive=4.5, click=0.16, punch=0.40, compressor=0.30, output=-5.5),
            R("Headroom", "A low-output clean utility preset for heavily processed sessions.", ["utility", "low-output"], body=0.06, harmonics=0.03, pitchDrop=11, pitchDecay=48, decay=1400, release=210, tone=1050, drive=0, click=0.04, punch=0.16, compressor=0.05, clipper=0.02, output=-9),
            R("Centered Low", "A solid centered body with restrained transient and medium decay.", ["centered", "restrained"], body=0.14, harmonics=0.13, pitchDrop=16, pitchDecay=44, decay=1150, release=170, tone=1450, drive=3, click=0.12, clickTone=3600, punch=0.34, compressor=0.24, output=-5),
            R("Final Bounce", "A punchy polished final-bounce 808 with predictable peak level.", ["polished", "predictable"], body=0.21, harmonics=0.22, pitchDrop=23, pitchDecay=32, pitchCurve=0.70, decay=760, release=105, tone=2550, drive=6, driveMode=2, click=0.32, clickTone=6700, punch=0.58, compressor=0.48, clipper=0.34, output=-6.5),
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
