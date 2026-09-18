#!/usr/bin/env bash
# Builds the signed, notarized macOS retail package for 808Glo Pro.
#
#   GLO_JUCE_DIR=/path/to/JUCE GLO_NOTARY_PROFILE=<keychain profile> ./scripts/release_macos.sh
#
# Produces, under dist/macos/:
#   808GloPro-<version>-macOS-Universal.pkg     signed, notarized, stapled
#   808GloPro-<version>-macOS.zip               retail download: INSTALL, licence, installer
#   *.sha256 and validation/BUILD_RECORD-<version>.txt
#
# Unlike build_macos.sh this does not need Xcode: it uses Unix Makefiles, which
# works on a Command Line Tools-only Mac. The version is read from CMakeLists.txt,
# so the package name, the bundles and the editor's version label cannot disagree.
#
# Signing identities are discovered from the keychain unless GLO_APP_IDENTITY /
# GLO_INSTALLER_IDENTITY are set. The notary profile is created once with
# `xcrun notarytool store-credentials`; no credential is ever read by this script.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
juce_dir="${GLO_JUCE_DIR:-${1:-}}"
notary_profile="${GLO_NOTARY_PROFILE:-}"
jobs="${GLO_JOBS:-2}"

if [[ -z "${juce_dir}" || ! -f "${juce_dir}/CMakeLists.txt" ]]; then
    echo "Set GLO_JUCE_DIR to a JUCE 9 checkout (or pass it as the first argument)." >&2
    exit 2
fi
if [[ -z "${notary_profile}" && "${GLO_SKIP_NOTARIZE:-0}" != "1" ]]; then
    echo "Set GLO_NOTARY_PROFILE to a notarytool keychain profile." >&2
    echo "A retail package must be notarized; GLO_SKIP_NOTARIZE=1 builds an internal one." >&2
    exit 2
fi

version="$(sed -n 's/^project(808GloPro VERSION \([0-9.]*\).*/\1/p' "${root}/CMakeLists.txt")"
[[ -n "${version}" ]] || { echo "Could not read the version from CMakeLists.txt" >&2; exit 1; }

find_identity() {
    security find-identity -v -p basic 2>/dev/null \
        | sed -n "s/.*\"\(${1}: [^\"]*\)\"/\1/p" | head -n 1
}
app_identity="${GLO_APP_IDENTITY:-$(find_identity "Developer ID Application")}"
installer_identity="${GLO_INSTALLER_IDENTITY:-$(find_identity "Developer ID Installer")}"
[[ -n "${app_identity}" ]] || { echo "No Developer ID Application identity found." >&2; exit 1; }
[[ -n "${installer_identity}" ]] || { echo "No Developer ID Installer identity found." >&2; exit 1; }

build="${root}/build/release-${version}"
dist="${root}/dist/macos"
validation="${root}/dist/validation/macos"
artefacts="${build}/808GloPro_artefacts/Release"
vst3="${artefacts}/VST3/808Glo Pro.vst3"
au="${artefacts}/AU/808Glo Pro.component"
app="${artefacts}/Standalone/808Glo Pro.app"
pkg="${dist}/808GloPro-${version}-macOS-Universal.pkg"
component_pkg="${dist}/808GloPro-${version}-component.pkg"
zip="${dist}/808GloPro-${version}-macOS.zip"
record="${validation}/BUILD_RECORD-${version}.txt"

step() { printf '\n==> %s\n' "$*"; }

step "808Glo Pro ${version}: configure a fresh universal build"
rm -rf "${build}"
cmake -S "${root}" -B "${build}" -G "Unix Makefiles" \
    -DJUCE_DIR="${juce_dir}" -DGLO_FETCH_JUCE=OFF \
    -DGLO_BUILD_PLUGIN=ON -DGLO_BUILD_TESTS=ON -DGLO_BUILD_TOOLS=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 > "${build}.configure.log"

step "build (-j ${jobs})"
targets=(808GloPro_All 808GloCoreTests 808GloJuceIntegrationTests)
# A dual-architecture JUCE build is memory-hungry; on a 16 GB machine clang is
# occasionally killed mid-file. That is memory pressure, not a code error, and a
# serial rerun gets through the same file, so retry once without parallelism.
if ! cmake --build "${build}" --target "${targets[@]}" -j "${jobs}" > "${build}.build.log" 2>&1; then
    echo "parallel build failed - retrying serially (see ${build}.build.log)"
    cmake --build "${build}" --target "${targets[@]}" >> "${build}.build.log" 2>&1
fi

step "test"
ctest --test-dir "${build}" --output-on-failure

step "check architectures and linkage"
for bundle in "${vst3}" "${au}" "${app}"; do
    binary="${bundle}/Contents/MacOS/808Glo Pro"
    archs="$(lipo -archs "${binary}")"
    [[ "${archs}" == *x86_64* && "${archs}" == *arm64* ]] \
        || { echo "${bundle} is not universal: ${archs}" >&2; exit 1; }
    if otool -L "${binary}" | tail -n +2 | grep -vqE '^\s+(/usr/lib/|/System/Library/)'; then
        echo "${bundle} links a non-system library:" >&2
        otool -L "${binary}" >&2
        exit 1
    fi
    echo "  ${archs}  $(basename "${bundle}")"
done

step "sign with ${app_identity}"
for bundle in "${vst3}" "${au}" "${app}"; do
    codesign --force --timestamp --options runtime --sign "${app_identity}" "${bundle}"
    codesign --verify --deep --strict "${bundle}"
done

step "stage installer payload"
stage="${dist}/stage"
rm -rf "${stage}"
mkdir -p "${stage}/Library/Audio/Plug-Ins/VST3" "${stage}/Library/Audio/Plug-Ins/Components" \
         "${stage}/Applications"
ditto --norsrc "${vst3}" "${stage}/Library/Audio/Plug-Ins/VST3/808Glo Pro.vst3"
ditto --norsrc "${au}" "${stage}/Library/Audio/Plug-Ins/Components/808Glo Pro.component"
ditto --norsrc "${app}" "${stage}/Applications/808Glo Pro.app"
# Extended attributes become ._ AppleDouble files inside the payload. The one
# attribute left afterwards, com.apple.provenance, cannot be removed and is fine.
xattr -cr "${stage}" 2>/dev/null || true
codesign --verify --deep --strict "${stage}/Applications/808Glo Pro.app"

step "build and sign the installer"
rm -f "${component_pkg}" "${pkg}"
pkgbuild --root "${stage}" --identifier "com.diamondloopz.808glopro.component" \
         --version "${version}" --install-location / "${component_pkg}"
productbuild --package "${component_pkg}" --sign "${installer_identity}" "${pkg}"
pkgutil --check-signature "${pkg}" | head -n 3
if pkgutil --payload-files "${component_pkg}" | grep -q '/\._'; then
    echo "AppleDouble files leaked into the payload" >&2
    exit 1
fi

notary_id="not notarized (internal build)"
if [[ "${GLO_SKIP_NOTARIZE:-0}" != "1" ]]; then
    step "notarize"
    mkdir -p "${validation}"
    xcrun notarytool submit "${pkg}" --keychain-profile "${notary_profile}" --wait \
        | tee "${validation}/notarization-${version}.log"
    grep -q "status: Accepted" "${validation}/notarization-${version}.log" \
        || { echo "Notarization was not accepted" >&2; exit 1; }
    notary_id="$(sed -n 's/^ *id: //p' "${validation}/notarization-${version}.log" | tail -n 1)"
    xcrun stapler staple "${pkg}"
    xcrun stapler validate "${pkg}"
    spctl --assess --type install --verbose=2 "${pkg}"
fi

step "assemble the retail download"
retail="${dist}/retail-${version}"
rm -rf "${retail}"
mkdir -p "${retail}/808Glo Pro/Installer"
sed "s/@VERSION@/${version}/g" "${root}/packaging/macos/INSTALL.txt" > "${retail}/808Glo Pro/INSTALL.txt"
sed "s/@VERSION@/${version}/g" "${root}/packaging/macos/license.txt" > "${retail}/808Glo Pro/license.txt"
cp "${pkg}" "${retail}/808Glo Pro/Installer/"
rm -f "${zip}"
ditto -c -k --norsrc --keepParent "${retail}/808Glo Pro" "${zip}"

# Hash AFTER stapling: stapling rewrites the package.
( cd "${dist}" && shasum -a 256 "$(basename "${pkg}")" > "$(basename "${pkg}").sha256" \
               && shasum -a 256 "$(basename "${zip}")" > "$(basename "${zip}").sha256" )

mkdir -p "${validation}"
{
    echo "808Glo Pro ${version} macOS retail build"
    echo "Source commit: $(git -C "${root}" rev-parse HEAD)$(git -C "${root}" diff --quiet || echo ' (DIRTY TREE)')"
    echo "JUCE: $(sed -n 's/.*version: *//p' "${juce_dir}/modules/juce_core/juce_core.h" | head -n 1) at ${juce_dir}"
    echo "Toolchain: $(clang++ --version | head -n 1); $(cmake --version | head -n 1)"
    echo "Architectures: arm64 + x86_64, deployment target 11.0"
    echo "Signed: ${app_identity} / ${installer_identity}"
    echo "Notarization: ${notary_id}"
    echo "pkg SHA-256: $(cut -d ' ' -f 1 "${pkg}.sha256")"
    echo "zip SHA-256: $(cut -d ' ' -f 1 "${zip}.sha256")"
} > "${record}"

step "done"
cat "${record}"
