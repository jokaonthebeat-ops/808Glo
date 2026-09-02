#!/usr/bin/env bash
set -euo pipefail

glo_script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
glo_project_dir="$(cd "${glo_script_dir}/.." && pwd)"
glo_output_dir="${TMPDIR:-/tmp}/808glo-core-qa"

mkdir -p "${glo_output_dir}"

g++ -std=c++20 -O2 \
    -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror \
    -I"${glo_project_dir}/Source" \
    "${glo_project_dir}/Tests/CoreDSPTests.cpp" \
    -o "${glo_output_dir}/808GloCoreTests"

"${glo_output_dir}/808GloCoreTests"

g++ -std=c++20 -O1 -g \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror \
    -I"${glo_project_dir}/Source" \
    "${glo_project_dir}/Tests/CoreDSPTests.cpp" \
    -o "${glo_output_dir}/808GloCoreTestsSanitized"

ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 \
UBSAN_OPTIONS=halt_on_error=1 \
    "${glo_output_dir}/808GloCoreTestsSanitized"

python3 "${glo_project_dir}/tools/validate_presets.py" \
    "${glo_project_dir}/Resources/FactoryPresets.json"

echo "Core DSP, sanitizers, and factory presets passed."

