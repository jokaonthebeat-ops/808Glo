# Building and Releasing 808Glo Pro on macOS

This guide produces the native macOS deliverables for 808Glo Pro:

- Universal VST3 (`arm64` and `x86_64`)
- Universal Audio Unit for Logic Pro
- Universal Standalone application
- Signed and notarized macOS installer package

Run these steps on a real Mac. A Linux build, Rosetta-only build, or unsigned local build is not a macOS release candidate.

## Fixed product identity

Do not change these values after release. DAW projects and Audio Unit discovery depend on them.

| Field | Value |
|---|---|
| Product | 808Glo Pro |
| Version | 1.0.0 |
| Company | Diamond Loopz |
| Bundle ID | `com.diamondloopz.808glopro` |
| Manufacturer code | `Dlpz` |
| Plug-in code / AU subtype | `Eglo` |
| AU type | `aumu` (`kAudioUnitType_MusicDevice`) |
| JUCE | 9.0.1 |
| C++ standard | C++20 |
| Minimum CMake | 3.22 |

## Release prerequisites

Install or provide all of the following:

- A currently supported macOS release on Apple Silicon.
- Xcode and its matching Command Line Tools.
- CMake 3.22 or newer.
- Python 3 for factory-preset validation.
- A local JUCE 9.0.1 source checkout.
- `pluginval` for plug-in validation.
- Steinberg's VST3 `validator` executable.
- An active Apple Developer Program membership.
- A `Developer ID Application` certificate.
- A `Developer ID Installer` certificate.
- An Apple notarization credential stored in Keychain.
- A commercial JUCE licence appropriate for this closed-source commercial release, unless the complete release is distributed in compliance with JUCE's applicable open-source licence.

Confirm the local tools before building:

```bash
xcodebuild -version
xcrun --show-sdk-path
cmake --version
python3 --version
security find-identity -v -p codesigning
```

JUCE must resolve to exactly 9.0.1. Do not build a release from `master` or silently accept JUCE 8 or a later JUCE 9 revision.

```bash
export JUCE_DIR="/absolute/path/to/JUCE"
test -f "$JUCE_DIR/CMakeLists.txt"
git -C "$JUCE_DIR" describe --tags --exact-match
```

The last command must report `9.0.1`. If JUCE was downloaded as an archive rather than cloned with Git, independently verify the archive version before proceeding.

## 1. Configure a clean universal build

Open Terminal at the 808Glo Pro project root:

```bash
cd /absolute/path/to/808GloPro

export GLO_ROOT="$PWD"
export GLO_BUILD="$GLO_ROOT/build/macos-release"
export GLO_VERSION="1.0.0"
export JUCE_DIR="/absolute/path/to/JUCE"
```

Use a new build directory for a release. Do not reuse a build tree created with another JUCE version, architecture, generator, or signing configuration.

```bash
cmake -S "$GLO_ROOT" -B "$GLO_BUILD" -G Xcode \
  -DJUCE_DIR:PATH="$JUCE_DIR" \
  -DGLO_FETCH_JUCE=OFF \
  -DGLO_BUILD_PLUGIN=ON \
  -DGLO_BUILD_TESTS=ON \
  -DGLO_BUILD_TOOLS=ON \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
```

Review the configure output. It must identify JUCE 9, Xcode, both macOS architectures, and no unexpected SDK or dependency fallback.

## 2. Validate presets and build

Validate the factory bank before compiling it into the plug-in:

```bash
python3 "$GLO_ROOT/tools/validate_presets.py" \
  "$GLO_ROOT/Resources/FactoryPresets.json"
```

The release bank must report 128 curated presets and exit with status 0.

Build the plug-in formats, Standalone application, core tests, and preview utility:

```bash
cmake --build "$GLO_BUILD" --config Release \
  --target 808GloPro_All 808GloCoreTests 808GloJuceIntegrationTests 808GloRenderPreview \
  --parallel
```

Run the dependency-free tests:

```bash
ctest --test-dir "$GLO_BUILD" -C Release --output-on-failure
```

Any compiler warning from 808Glo Pro code, test failure, or preset validation failure blocks release.

## 3. Locate and inspect the artifacts

JUCE should produce these paths:

```bash
export GLO_ARTEFACTS="$GLO_BUILD/808GloPro_artefacts/Release"
export GLO_VST3="$GLO_ARTEFACTS/VST3/808Glo Pro.vst3"
export GLO_AU="$GLO_ARTEFACTS/AU/808Glo Pro.component"
export GLO_APP="$GLO_ARTEFACTS/Standalone/808Glo Pro.app"

test -d "$GLO_VST3"
test -d "$GLO_AU"
test -d "$GLO_APP"
```

Verify that every binary is universal:

```bash
lipo -archs "$GLO_VST3/Contents/MacOS/808Glo Pro"
lipo -archs "$GLO_AU/Contents/MacOS/808Glo Pro"
lipo -archs "$GLO_APP/Contents/MacOS/808Glo Pro"
```

Each command must list both `x86_64` and `arm64`.

Inspect linked libraries and deployment targets:

```bash
otool -L "$GLO_VST3/Contents/MacOS/808Glo Pro"
otool -L "$GLO_AU/Contents/MacOS/808Glo Pro"
otool -L "$GLO_APP/Contents/MacOS/808Glo Pro"

otool -l "$GLO_VST3/Contents/MacOS/808Glo Pro" | grep -A 4 LC_BUILD_VERSION
```

There must be no accidental references to Homebrew, a developer home directory, or other non-system dynamic libraries that will be absent on a customer's Mac.

## 4. Sign the plug-in formats and application

Set the exact certificate identity shown by `security find-identity`. Do not put certificate secrets or notarization passwords in this repository.

```bash
export GLO_APP_IDENTITY="Developer ID Application: LEGAL NAME (TEAMID)"
```

All content and metadata changes must be finished before signing. Do not alter, recompress, inject, or rename files inside a signed bundle.

```bash
codesign --force --timestamp --options runtime \
  --sign "$GLO_APP_IDENTITY" "$GLO_VST3"

codesign --force --timestamp --options runtime \
  --sign "$GLO_APP_IDENTITY" "$GLO_AU"

codesign --force --timestamp --options runtime \
  --sign "$GLO_APP_IDENTITY" "$GLO_APP"
```

Verify all three signatures:

```bash
codesign --verify --deep --strict --verbose=2 "$GLO_VST3"
codesign --verify --deep --strict --verbose=2 "$GLO_AU"
codesign --verify --deep --strict --verbose=2 "$GLO_APP"

codesign --display --verbose=4 "$GLO_VST3"
codesign --display --verbose=4 "$GLO_AU"
codesign --display --verbose=4 "$GLO_APP"
```

The signatures must show the expected Team ID, timestamp, and hardened runtime. An ad-hoc signature is not a release signature.

## 5. Run VST3 validation before packaging

Create a durable log folder:

```bash
export GLO_VALIDATION="$GLO_ROOT/dist/validation/macos"
mkdir -p "$GLO_VALIDATION"
```

Run pluginval at strictness level 10 against the signed VST3:

```bash
export PLUGINVAL="/Applications/pluginval.app/Contents/MacOS/pluginval"
"$PLUGINVAL" --strictness-level 10 \
  --output-dir "$GLO_VALIDATION" \
  "$GLO_VST3"
```

Run Steinberg's official VST3 validator:

```bash
export VST3_VALIDATOR="/absolute/path/to/validator"
"$VST3_VALIDATOR" "$GLO_VST3" \
  > "$GLO_VALIDATION/vst3-validator.log" 2>&1
GLO_VALIDATOR_STATUS=$?
cat "$GLO_VALIDATION/vst3-validator.log"
test "$GLO_VALIDATOR_STATUS" -eq 0
```

Both programs must exit with status 0. Do not hide validator failures with `|| true`, and do not substitute a Standalone launch for plug-in validation.

## 6. Stage the installer payload

The factory bank is embedded in the binaries. Do not install a duplicate factory JSON file into a user-writable preset folder.

```bash
export GLO_DIST="$GLO_ROOT/dist/macos"
export GLO_STAGE="$GLO_DIST/stage"

test "$GLO_STAGE" = "$GLO_ROOT/dist/macos/stage"
rm -rf "$GLO_STAGE"

mkdir -p "$GLO_STAGE/Library/Audio/Plug-Ins/VST3"
mkdir -p "$GLO_STAGE/Library/Audio/Plug-Ins/Components"
mkdir -p "$GLO_STAGE/Applications"

ditto "$GLO_VST3" \
  "$GLO_STAGE/Library/Audio/Plug-Ins/VST3/808Glo Pro.vst3"
ditto "$GLO_AU" \
  "$GLO_STAGE/Library/Audio/Plug-Ins/Components/808Glo Pro.component"
ditto "$GLO_APP" \
  "$GLO_STAGE/Applications/808Glo Pro.app"
```

Confirm the staged payload contains exactly those three products and no build files, source files, signing credentials, or preview-only assets.

## 7. Build and sign the installer

Set the exact Installer certificate identity:

```bash
export GLO_INSTALLER_IDENTITY="Developer ID Installer: LEGAL NAME (TEAMID)"
export GLO_COMPONENT_PKG="$GLO_DIST/808GloPro-$GLO_VERSION-component.pkg"
export GLO_FINAL_PKG="$GLO_DIST/808GloPro-$GLO_VERSION-macOS-Universal.pkg"
```

Build the component package and signed product package:

```bash
pkgbuild \
  --root "$GLO_STAGE" \
  --identifier "com.diamondloopz.808glopro.component" \
  --version "$GLO_VERSION" \
  --install-location / \
  "$GLO_COMPONENT_PKG"

productbuild \
  --package "$GLO_COMPONENT_PKG" \
  --sign "$GLO_INSTALLER_IDENTITY" \
  "$GLO_FINAL_PKG"
```

Verify the installer signature and payload:

```bash
pkgutil --check-signature "$GLO_FINAL_PKG"
pkgutil --payload-files "$GLO_COMPONENT_PKG"
```

## 8. Notarize and staple

Store notarization credentials once in Keychain. Use an app-specific password or App Store Connect API credentials; never save credentials in a script or commit them.

```bash
xcrun notarytool store-credentials "DiamondLoopz-Notary" \
  --apple-id "APPLE_ID_EMAIL" \
  --team-id "TEAMID" \
  --password "APP_SPECIFIC_PASSWORD"
```

Submit the signed package and wait for acceptance:

```bash
xcrun notarytool submit "$GLO_FINAL_PKG" \
  --keychain-profile "DiamondLoopz-Notary" \
  --wait
```

The result must say `Accepted`. Then staple and validate the ticket:

```bash
xcrun stapler staple "$GLO_FINAL_PKG"
xcrun stapler validate "$GLO_FINAL_PKG"
spctl --assess --type install --verbose=4 "$GLO_FINAL_PKG"
```

A successful upload is not enough. Rejected, invalid, or unstapled packages must not be published.

## 9. Test the exact packaged installer

Use a clean QA Mac or disposable macOS account, not the build account. Install the exact notarized package intended for customers:

```bash
sudo installer -pkg "$GLO_FINAL_PKG" -target /
```

Confirm these installed paths:

```bash
test -d "/Library/Audio/Plug-Ins/VST3/808Glo Pro.vst3"
test -d "/Library/Audio/Plug-Ins/Components/808Glo Pro.component"
test -d "/Applications/808Glo Pro.app"
```

Run the validators again on the installed VST3, then validate the Audio Unit:

```bash
"$PLUGINVAL" --strictness-level 10 \
  "/Library/Audio/Plug-Ins/VST3/808Glo Pro.vst3"

"$VST3_VALIDATOR" \
  "/Library/Audio/Plug-Ins/VST3/808Glo Pro.vst3"

auval -v aumu Eglo Dlpz
```

Launch the Standalone app and complete the macOS items in `QA_CHECKLIST.md`. Test the AU in Logic Pro and the VST3 in at least two VST3 hosts.

## 10. Hash and archive the release candidate

```bash
shasum -a 256 "$GLO_FINAL_PKG" \
  | tee "$GLO_FINAL_PKG.sha256"
```

Archive the final package, checksum, CMake configure output, compiler version, JUCE tag, validator logs, notarization submission ID, and completed QA checklist. Do not archive passwords, private keys, certificate exports, or Keychain data.

## Current validation status

As of 2026-09-02, this workspace has not produced or tested a native macOS JUCE binary. Code signing, pluginval level 10, Steinberg validation, `auval`, host testing, installer testing, and notarization are pending. The package is not approved for public release until every applicable gate above and in `RELEASE_CHECKLIST.md` is complete.

## Authoritative references

- [JUCE CMake API](https://github.com/juce-framework/JUCE/blob/master/docs/CMake%20API.md)
- [JUCE 9.0.1 release](https://github.com/juce-framework/JUCE/releases/tag/9.0.1)
- [Apple notarization workflow](https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution)
- [Steinberg VST3 plug-in locations](https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical%2BDocumentation/Locations%2BFormat/Plugin%2BLocations.html)
- [pluginval](https://github.com/Tracktion/pluginval)
