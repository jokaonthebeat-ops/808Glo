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
# GLO_REUSE_BUILD=1 skips the compile when build/release-<version> was built from
# a commit whose Source/, Resources/ and CMakeLists.txt match the current tree -
# so a packaging-only fix does not cost another universal build. Tests and every
# check after them still run.
#
# Signing identities are discovered from the keychain unless GLO_APP_IDENTITY /
# GLO_INSTALLER_IDENTITY are set. The notary profile is created once with
# `xcrun notarytool store-credentials`; no credential is ever read by this script.
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
juce_dir="${GLO_JUCE_DIR:-${1:-}}"
notary_profile="${GLO_NOTARY_PROFILE:-}"
jobs="${GLO_JOBS:-2}"
min_macos="11.0"   # deployment target, the installer's OS gate and LSMinimumSystemVersion

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

# Captured up front: the record must name the commit the binaries came from, and
# the tree may legitimately move while a long build is running.
head_commit="$(git -C "${root}" rev-parse HEAD)"
build_paths=(Source Resources CMakeLists.txt)

reused=0
if [[ "${GLO_REUSE_BUILD:-0}" == "1" && -f "${build}/SOURCE_COMMIT" ]]; then
    built_from="$(cat "${build}/SOURCE_COMMIT")"
    if git -C "${root}" diff --quiet "${built_from}" -- "${build_paths[@]}"; then
        reused=1
    else
        echo "Sources changed since ${built_from}; the build cannot be reused." >&2
        exit 1
    fi
fi

if [[ "${reused}" == "1" ]]; then
    step "808Glo Pro ${version}: reusing the build from ${built_from}"
else
built_from="${head_commit}"
if ! git -C "${root}" diff --quiet HEAD -- "${build_paths[@]}"; then
    echo "Uncommitted changes under ${build_paths[*]}; commit before a release build." >&2
    exit 1
fi
step "808Glo Pro ${version}: configure a fresh universal build"
rm -rf "${build}"
cmake -S "${root}" -B "${build}" -G "Unix Makefiles" \
    -DJUCE_DIR="${juce_dir}" -DGLO_FETCH_JUCE=OFF \
    -DGLO_BUILD_PLUGIN=ON -DGLO_BUILD_TESTS=ON -DGLO_BUILD_TOOLS=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="${min_macos}" > "${build}.configure.log"

step "build (-j ${jobs})"
targets=(808GloPro_All 808GloCoreTests 808GloJuceIntegrationTests)
# A dual-architecture JUCE build is memory-hungry; on a 16 GB machine clang is
# occasionally killed mid-file. That is memory pressure, not a code error, and a
# serial rerun gets through the same file, so retry once without parallelism.
if ! cmake --build "${build}" --target "${targets[@]}" -j "${jobs}" > "${build}.build.log" 2>&1; then
    echo "parallel build failed - retrying serially (see ${build}.build.log)"
    cmake --build "${build}" --target "${targets[@]}" >> "${build}.build.log" 2>&1
fi
echo "${built_from}" > "${build}/SOURCE_COMMIT"
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

# Bundles must not be relocatable. All three share one CFBundleIdentifier (JUCE
# gives every format the same ID), and a relocatable bundle is installed wherever
# the system finds an existing copy with that ID - a stray copy in a Downloads or
# build folder would silently receive the update instead of /Library.
component_plist="${build}/component.plist"
pkgbuild --analyze --root "${stage}" "${component_plist}" > /dev/null
index=0
while /usr/libexec/PlistBuddy -c "Print :${index}" "${component_plist}" > /dev/null 2>&1; do
    # --analyze only writes the key for the .app; for the plug-in bundles it is
    # absent, and absent means relocatable. Set it where present, add it where not.
    /usr/libexec/PlistBuddy -c "Set :${index}:BundleIsRelocatable false" "${component_plist}" 2>/dev/null \
        || /usr/libexec/PlistBuddy -c "Add :${index}:BundleIsRelocatable bool false" "${component_plist}"
    index=$((index + 1))
done
pkgbuild --root "${stage}" --component-plist "${component_plist}" \
         --identifier "com.diamondloopz.808glopro.component" \
         --version "${version}" --install-location / "${component_pkg}"

# A distribution file gives the installer a title, shows the licence for the
# customer to accept, and refuses a macOS older than the binaries can run on:
# they hard-link macOS 11 APIs, so an older system would install a plug-in that
# silently fails to load.
resources="${build}/installer-resources"
rm -rf "${resources}"
mkdir -p "${resources}"
sed "s/@VERSION@/${version}/g" "${root}/packaging/macos/license.txt" > "${resources}/license.txt"
distribution="${build}/distribution.xml"
productbuild --synthesize --package "${component_pkg}" "${distribution}" > /dev/null
python3 - "${distribution}" "${version}" "${min_macos}" <<'PYEOF'
import sys
path, version, min_os = sys.argv[1:4]
xml = open(path).read()
head = '<installer-gui-script minSpecVersion="1">'
if head not in xml:
    sys.exit("unexpected productbuild --synthesize output: " + xml[:200])
extra = (f'\n    <title>808Glo Pro {version}</title>'
         f'\n    <license file="license.txt"/>'
         f'\n    <allowed-os-versions><os-version min="{min_os}"/></allowed-os-versions>')
open(path, "w").write(xml.replace(head, head + extra, 1))
PYEOF
productbuild --distribution "${distribution}" --resources "${resources}" \
             --package-path "${dist}" --sign "${installer_identity}" "${pkg}"
pkgutil --check-signature "${pkg}" | head -n 3

# Prove the installer carries what was just configured, rather than trusting it.
inspect="${build}/product-check"
rm -rf "${inspect}"
pkgutil --expand "${pkg}" "${inspect}"
grep -q "os-version min=\"${min_macos}\"" "${inspect}/Distribution" \
    || { echo "installer has no macOS ${min_macos} gate" >&2; exit 1; }
grep -q '<license file="license.txt"' "${inspect}/Distribution" \
    || { echo "installer has no licence pane" >&2; exit 1; }
# Relocation is declared per bundle in the <relocate> element, not by the
# top-level relocatable="..." attribute, which reads "false" even on a package
# whose bundles ARE relocatable. Any bundle listed there fails the release.
if python3 - "${inspect}" <<'PYEOF'
import glob, re, sys
info = open(glob.glob(sys.argv[1] + "/*.pkg/PackageInfo")[0]).read()
block = re.search(r"<relocate>(.*?)</relocate>", info, re.S)
sys.exit(0 if block and block.group(1).strip() else 1)
PYEOF
then
    echo "installer still lists a relocatable bundle" >&2
    exit 1
fi
rm -rf "${inspect}"
echo "  installer: macOS ${min_macos}+ gate, licence pane, no relocatable bundles"

# What matters is what lands on a customer's disk, not how the payload encodes
# it. macOS attaches com.apple.provenance to files built inside some sandboxed
# sessions and will not let it be removed, and pkgbuild records any extended
# attribute as a ._ entry. Installation folds those entries back into attributes,
# so the check is: expand the payload as the installer would, then require no
# stray ._ files and intact signatures on every bundle.
expanded="${build}/payload-check"
rm -rf "${expanded}"
pkgutil --expand-full "${component_pkg}" "${expanded}"
if [[ -n "$(find "${expanded}/Payload" -name '._*' -print -quit)" ]]; then
    echo "The installed payload would contain stray ._ files" >&2
    exit 1
fi
for bundle in "Library/Audio/Plug-Ins/VST3/808Glo Pro.vst3" \
              "Library/Audio/Plug-Ins/Components/808Glo Pro.component" \
              "Applications/808Glo Pro.app"; do
    codesign --verify --deep --strict "${expanded}/Payload/${bundle}" \
        || { echo "Installed signature would be broken: ${bundle}" >&2; exit 1; }
done
rm -rf "${expanded}"
echo "  payload expands cleanly; all three signatures verify as installed"

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
    # spctl exits 0 on any package when Gatekeeper assessments are disabled on
    # the build Mac (they are on this one), so its exit status proves nothing.
    # What it still reports is the SOURCE it would trust, and that is checked.
    assessment="$(spctl --assess --type install --verbose=2 "${pkg}" 2>&1 || true)"
    echo "${assessment}"
    grep -q "source=Notarized Developer ID" <<< "${assessment}" \
        || { echo "Gatekeeper does not see a notarized Developer ID package" >&2; exit 1; }
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
    echo "Binaries built from: ${built_from}$([[ "${reused}" == "1" ]] && echo ' (build reused)')"
    echo "Packaged at: ${head_commit}"
    echo "JUCE: $(sed -n 's/.*version: *//p' "${juce_dir}/modules/juce_core/juce_core.h" | head -n 1) at ${juce_dir}"
    echo "Toolchain: $(clang++ --version | head -n 1); $(cmake --version | head -n 1)"
    echo "Architectures: arm64 + x86_64, deployment target ${min_macos}; installer requires macOS ${min_macos}+"
    echo "Signed: ${app_identity} / ${installer_identity}"
    echo "Notarization: ${notary_id}"
    echo "pkg SHA-256: $(cut -d ' ' -f 1 "${pkg}.sha256")"
    echo "zip SHA-256: $(cut -d ' ' -f 1 "${zip}.sha256")"
} > "${record}"

step "done"
cat "${record}"
