#!/usr/bin/env bash
# Update expected.json files for golden tests with actual output.
# This script runs each test and captures its output as the new expected.json.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${ROOT}/cmake-build-debug"
GOLDEN_DIR="${ROOT}/tests/golden"
RUNTIME_HDR="${ROOT}/runtime"

# Use cmake-build-debug if it exists, otherwise build/
if [[ ! -d "${BUILD}" ]]; then
    BUILD="${ROOT}/build"
fi

# Plugin extension varies by platform
if [[ "$(uname)" == "Darwin" ]]; then
    PLUGIN="${BUILD}/libInspectorPlugin.dylib"
    CLANGXX="${CLANGXX:-/opt/homebrew/opt/llvm/bin/clang++}"
else
    PLUGIN="${BUILD}/libInspectorPlugin.so"
    CLANGXX="${CLANGXX:-clang++}"
fi

RUNTIME="${BUILD}/libinspector_runtime.a"

# Check dependencies
[[ -f "${PLUGIN}"  ]] || { echo "Plugin not built at ${PLUGIN}. Run cmake first."  >&2; exit 1; }
[[ -f "${RUNTIME}" ]] || { echo "Runtime not built at ${RUNTIME}. Run cmake first." >&2; exit 1; }

# Counters
UPDATED=0
FAILED=0
SKIPPED=0

# Update a single test
update_test() {
    local test_dir="$1"
    local test_name=$(basename "${test_dir}")
    local input="${test_dir}/input.cpp"
    local expected="${test_dir}/expected.json"

    if [[ ! -f "${input}" ]]; then
        echo "SKIP: ${test_name} (no input.cpp)"
        ((SKIPPED++))
        return 0
    fi

    local workdir=$(mktemp -d)
    trap 'rm -rf "${workdir}"' RETURN

    echo -n "UPDATE: ${test_name} ... "

    # Pass 1: Instrument
    if ! "${CLANGXX}" \
        -fsyntax-only \
        -fplugin="${PLUGIN}" \
        -I"${RUNTIME_HDR}" \
        -std=c++17 \
        "${input}" 2>"${workdir}/instrument.log"; then
        echo "FAIL (instrumentation)"
        cat "${workdir}/instrument.log"
        ((FAILED++))
        return 1
    fi

    local instrumented="${input}.instrumented.cpp"
    if [[ ! -f "${instrumented}" ]]; then
        echo "FAIL (no instrumented output)"
        ((FAILED++))
        return 1
    fi

    # Pass 2: Compile
    if ! "${CLANGXX}" \
        -I"${RUNTIME_HDR}" \
        -O0 -g \
        -std=c++17 \
        -c "${instrumented}" \
        -o "${workdir}/test.o" 2>"${workdir}/compile.log"; then
        echo "FAIL (compilation)"
        cat "${workdir}/compile.log"
        ((FAILED++))
        rm -f "${instrumented}"
        return 1
    fi

    # Pass 3: Link
    if ! "${CLANGXX}" \
        "${workdir}/test.o" \
        "${RUNTIME}" \
        -o "${workdir}/test" 2>"${workdir}/link.log"; then
        echo "FAIL (linking)"
        cat "${workdir}/link.log"
        ((FAILED++))
        rm -f "${instrumented}"
        return 1
    fi

    # Run
    "${workdir}/test" 2>"${workdir}/actual.json" || true

    # Check if output is valid JSON
    if ! python3 -c "import json; json.load(open('${workdir}/actual.json'))" 2>/dev/null; then
        echo "FAIL (invalid JSON output)"
        cat "${workdir}/actual.json"
        ((FAILED++))
        rm -f "${instrumented}"
        return 1
    fi

    # Pretty-print and save as expected.json
    python3 -c "
import json
with open('${workdir}/actual.json', 'r') as f:
    data = json.load(f)
with open('${expected}', 'w') as f:
    json.dump(data, f, indent=2)
"

    echo "OK"
    ((UPDATED++))

    # Clean up instrumented file
    rm -f "${instrumented}"
    return 0
}

# Main
echo "=== Updating expected.json files ==="
echo ""

# Find all test directories
for test_dir in "${GOLDEN_DIR}"/*; do
    if [[ -d "${test_dir}" ]]; then
        # Check for subdirectories (category/test structure)
        if ls "${test_dir}"/*/input.cpp >/dev/null 2>&1; then
            # Has subdirectories with tests
            for subdir in "${test_dir}"/*; do
                if [[ -d "${subdir}" ]]; then
                    update_test "${subdir}" || true
                fi
            done
        elif [[ -f "${test_dir}/input.cpp" ]]; then
            # Direct test directory
            update_test "${test_dir}" || true
        fi
    fi
done

echo ""
echo "=== Results ==="
echo "Updated: ${UPDATED}"
echo "Failed:  ${FAILED}"
echo "Skipped: ${SKIPPED}"
