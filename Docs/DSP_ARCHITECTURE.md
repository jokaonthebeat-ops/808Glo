# 808Glo Pro DSP Architecture

## Signal flow

```text
MIDI + keyboard
  → fixed mono/poly voice allocator
  → tuned fundamental + pitch envelope
  → phase-locked harmonics + click transient
  → per-voice tone filter + 5 Hz DC blocker
  → stereo voice sum
  → fixed 4× oversampled colour stage
  → stereo-linked compressor
  → output trim
  → 5 Hz post-colour DC blockers
  → soft clip + −0.8 dBFS emergency ceiling
  → output / meters / scope
```

## Voice engine

`Source/DSP` is JUCE-independent C++20. The processor owns one fixed `SynthEngine`, and the engine owns eight preallocated voices. No voice, buffer, string, file, or container allocation occurs during audio rendering.

Each voice contains:

- double-precision phase and current/target pitch
- deterministic xorshift click-noise state
- amplitude and pitch-envelope counters
- finite, exact-duration glide state in semitone space
- one-pole tone/click filters and a 5 Hz DC blocker
- MIDI note, voice age, velocity, and pitch-bend state

### Body generation

The fundamental is a deterministic sine. `BODY` morphs toward a rounded triangle character. `HARMONICS` adds phase-locked H2, H3, and H5 energy; `BALANCE` moves between odd and even emphasis. Harmonics fade before their partials reach the Nyquist guard band.

### Pitch drop

The transient pitch starts above the MIDI destination and decays by 60 dB across the displayed `TIME`. `DROP` is measured in semitones, and `CURVE` controls the motion without changing the final tuned destination.

### Glide

Glide interpolates in semitone/log-frequency space, not linear hertz. It finishes on the exact destination after the displayed number of milliseconds. Mono legato retargets pitch without resetting oscillator phase, click, or amplitude envelope.

### Trigger modes

- **One Shot:** note-off does not truncate the programmed body tail.
- **Gate:** the envelope decays toward sustain and enters release on note-off.

Mono mode keeps a fixed 128-note held stack and returns to the most recently held note with its original velocity. Poly mode uses a fixed eight-voice pool, prefers the quietest/releasing voice when stealing, and applies a short de-click crossfade.

## Nonlinear processing

All four colour modes run through JUCE's fixed 4× oversampling path, including at zero drive. That keeps latency constant across presets and automation.

- **Clean:** restrained soft boundary
- **Warm:** asymmetric saturation for added speaker translation
- **Hard:** anti-aliased hard clipping
- **Fold:** controlled wavefolding for aggressive presets

The oversampling latency is reported to the host. Output trim belongs before the final soft clip/ceiling.

## Real-time contract

The audio callback must never:

- allocate or free memory
- read/write files or parse presets
- lock a mutex
- copy a `ValueTree`
- call UI objects
- log to a console

APVTS atomic pointers are cached once and read with relaxed atomics. Host MIDI messages are rendered in segments at their sample offsets. UI-keyboard notes use a fixed lock-free SPSC queue with an overflow panic, avoiding JUCE keyboard locks and MIDI-buffer growth on the audio thread. Meter and scope publication use atomics and fixed storage.

## Future-safe rules

- Never rename or reuse a released parameter ID.
- Never change `Dlpz`, `Eglo`, or the bundle ID after release.
- Presets may not change oversampling or reported latency.
- New parameters require a parameter-version bump and state migration.
- Any future stereo enhancement must leave the low fundamental dual-mono.
