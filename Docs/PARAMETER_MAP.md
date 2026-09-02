# Parameter Map

All values below are raw APVTS units. IDs are the permanent automation/state contract.

| ID | Display | Range / choices | Default | Main UI |
|---|---|---|---:|---|
| `voiceMode` | Voice Mode | Mono, Poly | Mono | Voice |
| `legato` | Legato | Off, On | On | Voice |
| `triggerMode` | Trigger | One Shot, Gate | One Shot | Voice |
| `velocitySens` | Velocity | 0–100% | 55% | Voice |
| `tune` | Tune | −24–+24 st | 0 | Voice |
| `fine` | Fine | −100–+100 ct | 0 | Voice |
| `bendRange` | Bend Range | 1–24 st | 12 | Voice |
| `glide` | Glide | 0–2000 ms | 85 | Pitch |
| `body` | Body Shape | 0–1 | 0.08 | Body + Tone |
| `harmonics` | Harmonics | 0–1 | 0.08 | Body + Tone |
| `harmonicBalance` | Harmonic Balance | −1 odd to +1 even | 0 | Body + Tone |
| `pitchDrop` | Pitch Drop | 0–48 st | 19 | Pitch |
| `pitchDecay` | Pitch Time | 5–500 ms | 42 | Pitch |
| `pitchCurve` | Pitch Curve | 0.25–4 | 1.25 | Pitch |
| `attack` | Attack | 0.05–50 ms | 0.5 | Envelope |
| `hold` | Hold | 0–250 ms | 0 | Envelope |
| `decay` | Decay / Tail | 50–8000 ms | 1050 | Hero Tail |
| `sustain` | Sustain | 0–1 | 0 | Envelope |
| `release` | Release | 5–3000 ms | 85 | Envelope |
| `ampCurve` | Amp Curve | 0.25–4 | 1.7 | Envelope |
| `click` | Click | 0–1 | 0.12 | Transient |
| `clickTone` | Click Tone | 500–16000 Hz | 4200 | Transient |
| `clickDecay` | Click Length | 0.1–30 ms | 3.5 | Transient |
| `punch` | Punch | 0–1 | 0.30 | Hero Punch |
| `tone` | Tone | 45–18000 Hz | 6200 | Body + Tone |
| `toneKeytrack` | Tone Keytrack | 0–1 | 0.12 | Transient |
| `drive` | Drive / Glo | 0–36 dB | 3 | Hero Glo |
| `driveMode` | Drive Mode | Clean, Warm, Hard, Fold | Warm | Master |
| `compressor` | Compression / Glue | 0–1 | 0.22 | Master |
| `clipper` | Clip | 0–1 | 0.18 | Master |
| `output` | Output | −24–+12 dB | −3 | Master |

## Stable identity

Parameter version hint is `1`. Once v1.0 ships, do not rename, delete, reorder semantically, or reuse any ID for a different meaning. Add future controls under new IDs and migrate older state deliberately.

