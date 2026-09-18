# 808Glo Pro Production QA Checklist

Complete this checklist against the exact signed release-candidate binaries. Record the OS, host, hardware, build commit, tester, date, and links to logs. A check mark without reproducible evidence is not a completed release gate.

## Current verified status — 2026-09-02

| Area | Status | Evidence / limitation |
|---|---|---|
| Dependency-free DSP tests | Passed for current inspected source | `Tests/CoreDSPTests.cpp` was rebuilt directly with GCC 13.3, C++20, strict warnings-as-errors, and again with AddressSanitizer/UndefinedBehaviorSanitizer; both runs exit 0. LeakSanitizer is unavailable in this container because of its ptrace/proc restrictions. Any later source change invalidates this result until tests are rebuilt and rerun. |
| Factory preset structure | Passed | `tools/validate_presets.py` validates 128 curated presets; nearest pair differs across seven meaningful sound dimensions. |
| Preset category counts | Passed | Clean & Sub 12, Trap 14, Distorted 13, Drill 13, Detroit 12, West Coast 12, Long Glide 12, Short Punch 12, Experimental 12, Mix Ready 16. |
| Core preview render | Passed | The 12-second 48 kHz stereo preview renders finite audio at -0.999887 dBFS peak with 0.000026 DC offset; it is listening material, not host-validation evidence. |
| Full JUCE build | Passed on Linux development host | JUCE 9.0.1 Release VST3 and Standalone targets built on x86-64 Linux. The processor integration test loads all 128 presets, checks state recall, oversized blocks, UI MIDI overflow recovery, latency, and output ceiling. This does not replace native macOS/Windows release builds. |
| Actual JUCE UI render | Passed on Linux development host | The real editor was rendered at 1152×720, 1440×900, 1600×1000, and 1920×1200. No label, knob, value, logo, preset-bar, Master-panel, scope, or keyboard overlap/clipping was observed. Native Retina and Windows DPI checks remain release-machine gates. |
| Native macOS build | Not run | Universal architecture, signatures, hosts, AU and notarization remain untested. |
| Native Windows build | Not run | MSVC build, signatures, hosts and installer remain untested. |
| pluginval level 10 | Not run | The validator is not installed in this workspace; run it against both native release candidates. |
| Steinberg VST3 validator | Not run | The validator is not installed in this workspace; run it against both native VST3 candidates. |
| Apple `auval` | Not run | No native AU has been produced. |
| Public release approval | Blocked | Native builds, validators, host tests, signatures and installer tests are mandatory. |

## Release-candidate record

- [ ] Version is `1.0.1` everywhere.
- [ ] Source commit/tag: ______________________________
- [ ] JUCE tag is exactly `9.0.1`.
- [ ] CMake version: ______________________________
- [ ] macOS Xcode/SDK version: ______________________________
- [ ] Windows Visual Studio/MSVC/SDK version: ______________________________
- [ ] macOS release filename: `808GloPro-1.0.1-macOS-Universal.pkg`
- [ ] Windows release filename: `808GloPro-1.0.1-Windows-x64-Setup.exe`
- [ ] Tester and date: ______________________________
- [ ] Validator and test logs are archived with this candidate.

## Identity and compatibility invariants

- [ ] Product name remains `808Glo Pro`.
- [ ] Company remains `Diamond Loopz`.
- [ ] Bundle ID remains `com.diamondloopz.808glopro`.
- [ ] Manufacturer code remains `Dlpz`.
- [ ] Plug-in code / AU subtype remains `Eglo`.
- [ ] AU type remains `aumu`.
- [ ] VST3 categories include Instrument, Synth and Drum.
- [ ] The 1.x parameter IDs, meanings, order and normalized ranges have not changed incompatibly.
- [ ] Old 1.x sessions and saved states load with the same audible result.
- [ ] The Windows Installer AppId remains `{347184D2-0C31-4D6F-82AF-1A7A3ACE6C29}`.

## Licence and third-party review

- [ ] Diamond Loopz has confirmed the JUCE licence applicable to this closed-source commercial binary.
- [ ] Disabling the JUCE splash screen is permitted by that licence.
- [ ] JUCE 9.0.1 licence notices and any required third-party notices are included.
- [ ] Every font, icon, texture, audio file and other shipped asset has documented commercial distribution rights.
- [ ] No source, test asset, credential, private key, certificate, customer data or unlicensed sample is inside the installer.

## Automated source, DSP and preset checks

- [ ] Clean configure completes without fallback to an unintended JUCE or SDK.
- [ ] Release build completes with no 808Glo Pro compiler warnings.
- [ ] `ctest --output-on-failure` passes in Release on macOS.
- [ ] `ctest --output-on-failure` passes in Release on Windows.
- [ ] Debug build passes on macOS and Windows.
- [ ] AddressSanitizer and UndefinedBehaviorSanitizer tests pass on a supported development configuration.
- [ ] Preset validator reports exactly 128 presets.
- [ ] Every preset has a unique name and stable ID.
- [ ] Every preset value is finite and within the declared parameter range.
- [ ] Every category is present and category counts match the approved bank.
- [ ] Loading every factory preset succeeds without exceptions, assertions or missing data.
- [ ] Factory presets are embedded and work without an external JSON file beside the plug-in.

## Audio engine correctness

Test at 44.1, 48, 88.2, 96 and 192 kHz. Test block sizes 1, 7, 16, 32, 64, 128, 256, 511, 512, 1024 and 2048 where the host permits them.

- [ ] Silence is truly silent before the first MIDI event.
- [ ] MIDI note 69 resolves to 440 Hz when pitch envelopes and tuning offsets are disabled.
- [ ] Notes 24–60 settle within ±1 cent after the designed pitch transient.
- [ ] Transpose, fine tune and pitch bend reach their exact intended endpoints.
- [ ] Pitch envelope depth, curve and time remain stable across sample rates.
- [ ] Glide lands exactly at its target at the displayed glide time.
- [ ] Legato does not restart or zero the amplitude envelope.
- [ ] Hard retrigger is click-controlled and repeatable.
- [ ] Attack, hold, decay and release times are sample-rate independent.
- [ ] One-shot mode terminates naturally and cannot create an immortal voice.
- [ ] Gated mode responds correctly to note-off, sustain pedal and all-notes-off.
- [ ] Maximum active voice count never exceeds eight.
- [ ] Voice stealing is deterministic and does not produce an uncontrolled discontinuity.
- [ ] Repeated same-note events and stale note-offs do not stop a reassigned voice.
- [ ] Switching mono/poly mode cannot revive a stale hidden voice.
- [ ] Mono held-note priority returns to the correct most-recent held note and retains its original velocity.
- [ ] Left and right outputs null for the mono 808 signal.
- [ ] There is no accidental +6 dB gain from dual-mono summing.
- [ ] No test render contains NaN, infinity, subnormal storms or unstable filter output.
- [ ] DC offset remains acceptably low without audibly thinning the sub fundamental.
- [ ] Output remains bounded at maximum velocity, maximum drive and eight voices.
- [ ] Offline bounce and real-time playback match unless a deliberately random mode is documented.
- [ ] Identical MIDI and state produce deterministic output.

## MIDI and host-event handling

- [ ] Note-on and note-off work at every sample offset within a block.
- [ ] Multiple MIDI events at the same sample offset behave deterministically.
- [ ] Velocity 1–127 has a useful, monotonic response.
- [ ] Pitch bend center, minimum and maximum are correct.
- [ ] Sustain pedal down/up, all-notes-off and all-sound-off work.
- [ ] Transport stop, seek, loop and host suspend/resume do not leave stuck audio.
- [ ] Repeated `prepareToPlay`/release cycles do not leak or retain invalid state.
- [ ] Sample-rate and block-size changes while the instance exists recover cleanly.

## Parameters, automation and state recall

- [ ] Every visible control writes the correct parameter.
- [ ] Every parameter automates in FL Studio, Ableton Live and at least one additional VST3 host.
- [ ] AU automation works in Logic Pro.
- [ ] Rapid automation at minimum/maximum values produces no NaN, crash or zipper burst.
- [ ] Smoothed parameters reach their target without an audible lag that contradicts the UI.
- [ ] Host bypass and plug-in power/bypass behavior are correct.
- [ ] Saving and reopening a DAW session restores every parameter and the selected preset.
- [ ] Copying state between two instances produces the same audible result.
- [ ] Malformed, empty and older state data fail safely.
- [ ] Preset previous/next controls do not skip, duplicate or leave stale labels.
- [ ] User preset save/load works in a user-writable location without administrator permission.
- [ ] Updating or uninstalling the product does not delete user-created presets.

## UI and accessibility

- [ ] Editor opens, closes and reopens repeatedly without a crash or dangling attachment.
- [ ] Closing a project with the editor open does not crash the host.
- [ ] UI remains responsive while audio plays at a 32-sample buffer.
- [ ] Preset browser, category filter, search, and previous/next behavior work as designed.
- [ ] All controls show clear labels, values, units, hover states and focus states.
- [ ] Keyboard focus does not steal normal DAW transport or editing shortcuts.
- [ ] Tab/focus navigation is sensible where supported.
- [ ] Text remains readable at every supported UI scale.
- [ ] Windows scaling passes at 100%, 125%, 150%, 175% and 200%.
- [ ] macOS passes on Retina and non-Retina/external displays.
- [ ] Resizing reaches declared minimum and maximum sizes without overlap, clipping or blank regions.
- [ ] UI survives moving between monitors with different scale factors.
- [ ] No temporary placeholder, debug label, missing image or developer path is visible.
- [ ] Meters animate accurately and return to rest.
- [ ] Reduced-motion or nonessential animation behavior does not affect audio or control access.

## Performance and real-time safety

- [ ] `processBlock` performs no heap allocation, file access, preset parsing, lock acquisition, logging or message-thread call.
- [ ] pluginval real-time safety tests pass where supported.
- [ ] Dense eight-voice MIDI at maximum drive completes without dropouts at 48 kHz / 64 samples on the documented minimum machine.
- [ ] CPU use is measured and recorded for 1, 8 and 16 plug-in instances.
- [ ] Standalone idles without abnormal CPU or GPU use.
- [ ] Opening/closing the editor does not change audio-thread stability.
- [ ] No memory growth appears during a 30-minute automation and preset-switching soak test.
- [ ] No denormal CPU spike occurs during long releases into silence.

## Binary validators — hard gates

- [ ] macOS VST3 passes pluginval strictness level 10 with exit status 0.
- [ ] Windows VST3 passes pluginval strictness level 10 with exit status 0.
- [ ] macOS VST3 passes Steinberg's VST3 validator with exit status 0.
- [ ] Windows VST3 passes Steinberg's VST3 validator with exit status 0.
- [ ] AU passes `auval -v aumu Eglo Dlpz`.
- [ ] Validator logs are stored with the exact candidate hashes.
- [ ] No validator was run against a different build than the packaged artifact.

## Required host matrix

Record the exact host and OS versions. Test the latest supported host release available on the QA date and one prior major release where practical.

| OS | Format | Host | Scan | Instantiate | Audio/MIDI | Automation | State recall | UI | Result/log |
|---|---|---|---|---|---|---|---|---|---|
| Windows 11 x64 | VST3 | FL Studio | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | |
| Windows 11 x64 | VST3 | Ableton Live | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | |
| Windows 11 x64 | VST3 | REAPER | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | |
| Windows 11 x64 | VST3 | Cubase or Studio One | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | |
| macOS Apple Silicon native | AU | Logic Pro | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | |
| macOS Apple Silicon native | VST3 | Ableton Live | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | |
| macOS Apple Silicon native | VST3 | REAPER | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | |
| macOS under Rosetta | VST3 | One x86_64-capable host | [ ] | [ ] | [ ] | [ ] | [ ] | [ ] | |

Also launch and test the Standalone application on both operating systems, including audio-device selection, MIDI-device input and clean shutdown.

## Preset sound and mix-readiness review

Audition all presets at multiple velocities and notes, not only the default note.

- [ ] Every preset has a clear production purpose and audible distinction from its nearest neighbor.
- [ ] Presets are gain-matched enough that louder is not mistaken for better.
- [ ] Clean presets retain a strong fundamental without unintended buzz.
- [ ] Driven presets retain the sub while translating to small speakers.
- [ ] Glide presets reach their target accurately during overlapping notes.
- [ ] Short presets remain punchy in fast patterns without clicks.
- [ ] Long presets decay smoothly without DC buildup, beating or uncontrolled rumble.
- [ ] Experimental presets remain gain-controlled and are clearly named.
- [ ] Presets are checked on full-range monitors, headphones, mono, a phone/small-speaker simulation and a 100 Hz high-pass audition.
- [ ] No preset clips unintentionally when triggered at velocity 127.

## Installer, update and uninstall QA

### macOS

- [ ] Installer has a valid Developer ID Installer signature.
- [ ] Apple notarization status is Accepted.
- [ ] Stapler validation passes without network access.
- [ ] Gatekeeper assessment passes.
- [ ] Installer places VST3, AU and Standalone only in their documented system locations.
- [ ] A clean user can scan and run all formats without developer tools installed.
- [ ] Installing over an earlier 1.x build updates binaries and preserves user presets.
- [ ] Manual removal instructions are verified.

### Windows

- [ ] VST3 inner binary, Standalone application and installer have valid trusted signatures and timestamps.
- [ ] Installer places the complete VST3 bundle in `C:\Program Files\Common Files\VST3`.
- [ ] Standalone is installed below `C:\Program Files\Diamond Loopz\808Glo Pro`.
- [ ] A standard user can run the installed products after administrator installation.
- [ ] Installing over an earlier 1.x build upgrades rather than duplicates the product.
- [ ] Uninstall removes installed program files and shortcuts.
- [ ] Uninstall preserves user-created presets.
- [ ] No reboot is required.

## Final QA sign-off

- [ ] All hard gates passed.
- [ ] All failures have a resolved issue reference and were retested on the final candidate.
- [ ] macOS package SHA-256: ______________________________
- [ ] Windows installer SHA-256: ______________________________
- [ ] macOS QA owner/date: ______________________________
- [ ] Windows QA owner/date: ______________________________
- [ ] Audio/preset QA owner/date: ______________________________
- [ ] Release owner/date: ______________________________

If any hard gate is incomplete, the correct status is **not ready for public release**.
