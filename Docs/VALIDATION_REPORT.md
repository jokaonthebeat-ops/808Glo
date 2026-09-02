# 808Glo Pro Validation Report

Validation date: 2026-09-02  
Source version: 1.0.0  
Framework target: JUCE 9.0.1 / C++20

## Completed in Codex

- Release-mode JUCE build completed for VST3 and Standalone on x86-64 Linux with GCC 13.3.
- All project C++ sources compiled with JUCE's recommended warning set; project-owned warnings were resolved.
- Three CTest targets passed: core DSP, strict factory preset validation, and JUCE processor integration.
- Core DSP passed warnings-as-errors plus AddressSanitizer and UndefinedBehaviorSanitizer.
- Stress coverage ran at 44.1, 48, 88.2, 96, and 192 kHz.
- The runtime integration test loaded all 128 embedded presets, checked 31 stable parameters, rendered a 4096-sample block after a 64-sample prepare size, verified finite output and the post-reconstruction ceiling, and round-tripped DAW state.
- The UI-keyboard path passed normal input and forced-overflow recovery without modifying the host MIDI buffer.
- The deterministic preset generator and validator passed the required category totals and sound-separation checks.
- The 12-second audio preview is 48 kHz, stereo, 16-bit PCM with a measured peak of -0.999887 dBFS and DC offset of 0.000026.
- The actual JUCE editor was rendered and visually inspected at 1152×720, 1440×900, 1600×1000, and 1920×1200. Labels, knobs, values, the 2×2 Master grid, preset strip, keyboard, meters, and scope remained clear without overlap or clipping.
- The canonical 808Glo Pro logo is an embedded, fully outlined transparent SVG. Runtime and documentation renders use the same asset, without relying on an installed font.
- Parameter readouts use compact unit-aware formatting, including percent, milliseconds/seconds, hertz/kilohertz, decibels, semitones, and cents.

## Release gates still required

The source is build-verified, but public binaries still require native release work:

- Xcode universal macOS VST3/AU/Standalone builds
- Visual Studio 2022 Windows x64 VST3/Standalone builds
- pluginval, Steinberg VST3 validator, `auval`, and the documented DAW host matrix
- Authenticode signing, Apple Developer ID signing/notarization, and installer QA
- final JUCE commercial-licence review for Diamond Loopz

Use `BUILD_MACOS.md`, `BUILD_WINDOWS.md`, `QA_CHECKLIST.md`, and `RELEASE_CHECKLIST.md` as the release gates. Do not distribute an unsigned development artifact as a public release.
