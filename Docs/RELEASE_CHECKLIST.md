# 808Glo Pro Release Checklist

This is the final go/no-go list for 808Glo Pro. Use `BUILD_MACOS.md`, `BUILD_WINDOWS.md`, and `QA_CHECKLIST.md` for the detailed commands and test cases.

## Non-negotiable release rule

Do not publicly distribute an unsigned binary, an unnotarized macOS package, an unvalidated VST3, or an installer that was not tested after signing. A build that sounds good locally is not a release candidate until every hard gate below passes.

## Current status — 2026-09-02

The current source passes strict GCC 13.3 core compilation, AddressSanitizer/UndefinedBehaviorSanitizer, all 128 factory-preset checks, and a full JUCE 9.0.1 Release VST3/Standalone build plus processor integration tests on x86-64 Linux. LeakSanitizer cannot run in this container. No native macOS or Windows JUCE release binary has been produced in this workspace. Pluginval, Steinberg VST3 validation, `auval`, DAW testing, signing, notarization, and installer QA remain pending. Public release is blocked.

## 1. Freeze identity and scope

- [ ] Release version is approved: `1.0.1`.
- [ ] Product name is `808Glo Pro`.
- [ ] Company is `Diamond Loopz`.
- [ ] Bundle ID is `com.diamondloopz.808glopro`.
- [ ] Manufacturer code is `Dlpz`.
- [ ] Plug-in code / AU subtype is `Eglo`.
- [ ] Windows Installer AppId is `{347184D2-0C31-4D6F-82AF-1A7A3ACE6C29}`.
- [ ] Parameter IDs and meanings are frozen for 1.x session compatibility.
- [ ] Factory preset count is frozen at 128.
- [ ] Release source commit/tag is recorded: ______________________________

Changing a plug-in code, manufacturer code, bundle ID, parameter ID, or Installer AppId after release can break session recall, plug-in discovery, automation, or upgrades.

## 2. Confirm legal and licence readiness

- [ ] Commercial JUCE 9 use has been reviewed and approved for Diamond Loopz.
- [ ] `JUCE_DISPLAY_SPLASH_SCREEN=0` is allowed under the selected licence.
- [ ] JUCE and third-party notices required by the selected licences are included.
- [ ] All visual, font and audio assets have commercial distribution rights.
- [ ] Product name, logo and bundle identifiers have completed internal trademark/name review.
- [ ] Privacy policy and EULA are included if the released product or installer requires them.
- [ ] No credential, signing key, PFX, password, customer data or restricted SDK is in the package.

## 3. Create reproducible clean builds

- [ ] JUCE source is exactly tag `9.0.1`.
- [ ] CMake is 3.22 or newer.
- [ ] macOS build uses Xcode and contains both `arm64` and `x86_64`.
- [ ] Windows build uses native MSVC x64.
- [ ] `GLO_FETCH_JUCE=OFF` is used with the reviewed local JUCE source for the final build.
- [ ] Build directories were created fresh for this candidate.
- [ ] Release configure logs contain no unexpected SDK, architecture or dependency fallback.
- [ ] Release builds complete without 808Glo Pro compiler warnings.
- [ ] Release binaries contain no developer-only or debug dynamic-library dependency.

## 4. Pass automated tests and preset validation

- [ ] macOS Release `ctest` passes.
- [ ] Windows Release `ctest` passes.
- [ ] Debug tests pass on both native platforms.
- [ ] Sanitizer run passes on a supported development build.
- [ ] `tools/validate_presets.py` reports exactly 128 valid curated presets.
- [ ] Every embedded preset loads in the JUCE plug-in.
- [ ] State save/load and preset recall tests pass.
- [ ] Offline/real-time render comparison passes.
- [ ] All applicable items in the automated, DSP, MIDI and state sections of `QA_CHECKLIST.md` pass.

## 5. Validate native plug-in binaries

- [ ] macOS VST3 passes pluginval strictness level 10.
- [ ] Windows VST3 passes pluginval strictness level 10.
- [ ] macOS VST3 passes Steinberg's VST3 validator.
- [ ] Windows VST3 passes Steinberg's VST3 validator.
- [ ] AU passes `auval -v aumu Eglo Dlpz`.
- [ ] Validation was run against the same signed binaries placed in the installers.
- [ ] Validator output and exit status are archived.

No validator failure may be waived by lowering strictness, skipping the failing test, or validating a different build.

## 6. Complete host and audio QA

- [ ] FL Studio VST3 test passes on Windows.
- [ ] Ableton Live VST3 tests pass on Windows and macOS.
- [ ] Logic Pro AU test passes natively on Apple Silicon.
- [ ] REAPER VST3 tests pass on Windows and macOS.
- [ ] At least one of Cubase or Studio One passes on Windows.
- [ ] One macOS x86_64/Rosetta compatibility test passes.
- [ ] Standalone passes on Windows and macOS.
- [ ] UI scaling, editor reopen, automation and session recall pass in the required hosts.
- [ ] All 128 presets complete the approved mix-readiness review.
- [ ] Thirty-minute stress/soak test passes without crash, leak, stuck voice or dropout.
- [ ] Completed host matrix and audio QA notes are archived.

## 7. Sign release binaries

### macOS

- [ ] VST3 is signed with Developer ID Application, timestamp and hardened runtime.
- [ ] AU is signed with Developer ID Application, timestamp and hardened runtime.
- [ ] Standalone app is signed with Developer ID Application, timestamp and hardened runtime.
- [ ] `codesign --verify --deep --strict` passes for all three.
- [ ] Team ID and certificate identity are correct.

### Windows

- [ ] VST3 inner binary has a valid SHA-256 Authenticode signature and timestamp.
- [ ] Standalone `.exe` has a valid SHA-256 Authenticode signature and timestamp.
- [ ] `signtool verify /pa /all /v` passes for both.
- [ ] PowerShell reports `Status : Valid` for both.

An ad-hoc macOS signature, self-signed Windows certificate, expired/untrusted chain, missing timestamp, or unsigned binary blocks release.

## 8. Build, sign and validate installers

### macOS package

- [ ] Filename is `808GloPro-1.0.1-macOS-Universal.pkg`.
- [ ] Payload contains VST3, AU and Standalone in the documented system locations.
- [ ] Package is signed with Developer ID Installer.
- [ ] `pkgutil --check-signature` passes.
- [ ] Apple notarization returns `Accepted`.
- [ ] Notarization ticket is stapled and `stapler validate` passes.
- [ ] Gatekeeper assessment passes.
- [ ] Package installs successfully on a clean Mac without development tools.
- [ ] Installed binaries pass pluginval, Steinberg validator and `auval` again.

### Windows installer

- [ ] Filename is `808GloPro-1.0.1-Windows-x64-Setup.exe`.
- [ ] Installer contains the complete VST3 bundle and Standalone app.
- [ ] Installer has a valid trusted SHA-256 Authenticode signature and timestamp.
- [ ] Fresh install passes on a clean Windows 11 x64 machine.
- [ ] Upgrade over an earlier 1.x installation passes without duplication.
- [ ] Uninstall removes program files and preserves user presets.
- [ ] Installed VST3 passes pluginval and Steinberg validator again.
- [ ] Installed Standalone launches and exits cleanly.

## 9. Verify customer-facing package contents

- [ ] Factory presets are available immediately after installation.
- [ ] No duplicate external factory bank is required.
- [ ] User manual and installation/troubleshooting instructions match the final filenames and paths.
- [ ] macOS instructions mention both AU and VST3 rescanning.
- [ ] Windows instructions use `C:\Program Files\Common Files\VST3`.
- [ ] System requirements are accurate and tested.
- [ ] Version number is correct in binaries, installers, product page and documentation.
- [ ] Product screenshots show the final UI.
- [ ] No placeholder copy, debug asset, internal note or preview-only file is shipped.

## 10. Generate hashes and archive evidence

- [ ] macOS package SHA-256 generated and verified: ______________________________
- [ ] Windows installer SHA-256 generated and verified: ______________________________
- [ ] Release artifacts uploaded, downloaded again, and their hashes rechecked.
- [ ] Release archive includes exact binaries, hashes, source tag, JUCE tag, toolchain versions, build logs, test logs, validator logs and notarization submission ID.
- [ ] Archive excludes all signing secrets and credentials.
- [ ] Previous public release remains available internally for rollback.

## 11. Final sign-off

- [ ] Engineering owner: ______________________________ Date: ______________
- [ ] macOS QA owner: ______________________________ Date: ______________
- [ ] Windows QA owner: ______________________________ Date: ______________
- [ ] Preset/audio QA owner: ______________________________ Date: ______________
- [ ] Legal/licence owner: ______________________________ Date: ______________
- [ ] Release owner: ______________________________ Date: ______________
- [ ] Every blocker is closed against the final artifact hashes.
- [ ] Final status is explicitly marked **Approved for public release**.

Until all hard gates and signatures are complete, label the build **internal preview only** and do not upload it to a public store, customer download, email campaign, or affiliate platform.
