#!/usr/bin/env bash
set -euo pipefail

glo_script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
glo_project_dir="$(cd "${glo_script_dir}/.." && pwd)"
glo_juce_dir="${1:-${GLO_JUCE_DIR:-}}"
glo_build_dir="${2:-${glo_project_dir}/build/macos}"

if [[ -z "${glo_juce_dir}" || ! -f "${glo_juce_dir}/CMakeLists.txt" ]]; then
    echo "Usage: $0 /absolute/path/to/JUCE [build-directory]" >&2
    echo "You may also set GLO_JUCE_DIR." >&2
    exit 2
fi

cmake -S "${glo_project_dir}" -B "${glo_build_dir}" -G Xcode \
    -DJUCE_DIR="${glo_juce_dir}" \
    -DGLO_FETCH_JUCE=OFF \
    -DGLO_BUILD_PLUGIN=ON \
    -DGLO_BUILD_TESTS=ON \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0

cmake --build "${glo_build_dir}" --config Release \
    --target 808GloPro_All 808GloCoreTests 808GloJuceIntegrationTests 808GloRenderPreview --parallel
ctest --test-dir "${glo_build_dir}" -C Release --output-on-failure

echo
echo "Build finished. Unsigned artifacts are under:"
echo "${glo_build_dir}/808GloPro_artefacts/Release"
echo "Follow Docs/BUILD_MACOS.md before distributing them."
