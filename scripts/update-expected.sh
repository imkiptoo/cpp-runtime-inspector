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

# Malloc shim for tests that need it
if [[ "$(uname)" == "Darwin" ]]; then
    MALLOC_SHIM="${BUILD}/libinspector_malloc_shim.dylib"
else
    MALLOC_SHIM="${BUILD}/libinspector_malloc_shim.so"
fi

# Check dependencies
[[ -f "${PLUGIN}"  ]] || { echo "Plugin not built at ${PLUGIN}. Run cmake first."  >&2; exit 1; }
[[ -f "${RUNTIME}" ]] || { echo "Runtime not built at ${RUNTIME}. Run cmake first." >&2; exit 1; }
HAVE_MALLOC_SHIM=0
[[ -f "${MALLOC_SHIM}" ]] && HAVE_MALLOC_SHIM=1

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

    # Use platform-specific expected file if it exists
    if [[ "$(uname)" == "Darwin" && -f "${test_dir}/expected.darwin.json" ]]; then
        expected="${test_dir}/expected.darwin.json"
    elif [[ "$(uname)" == "Linux" && -f "${test_dir}/expected.linux.json" ]]; then
        expected="${test_dir}/expected.linux.json"
    fi

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
    # See run-golden-tests.sh for the rationale behind export flags.
    LINK_EXTRA=()
    if [[ "$(uname)" == "Darwin" ]]; then
        LINK_EXTRA+=(-Wl,-export_dynamic)
    else
        LINK_EXTRA+=(-rdynamic -ldl)
    fi
    if ! "${CLANGXX}" \
        "${workdir}/test.o" \
        "${RUNTIME}" \
        ${LINK_EXTRA[@]+"${LINK_EXTRA[@]}"} \
        -o "${workdir}/test" 2>"${workdir}/link.log"; then
        echo "FAIL (linking)"
        cat "${workdir}/link.log"
        ((FAILED++))
        rm -f "${instrumented}"
        return 1
    fi

    # Run - check if test needs malloc shim
    local use_shim=0
    if [[ -f "${test_dir}/.use_shim" ]]; then
        use_shim=1
        if [[ ${HAVE_MALLOC_SHIM} -eq 0 ]]; then
            echo "SKIP (needs malloc shim, not built)"
            ((SKIPPED++))
            rm -f "${instrumented}"
            return 0
        fi
    fi

    # Optional CLI args from .args file (whitespace-separated).
    local -a test_args=()
    if [[ -f "${test_dir}/.args" ]]; then
        read -r -a test_args < "${test_dir}/.args"
    fi

    if [[ ${use_shim} -eq 1 ]]; then
        if [[ "$(uname)" == "Darwin" ]]; then
            DYLD_INSERT_LIBRARIES="${MALLOC_SHIM}" "${workdir}/test" ${test_args[@]+"${test_args[@]}"} 2>"${workdir}/actual.json" || true
        else
            LD_PRELOAD="${MALLOC_SHIM}" "${workdir}/test" ${test_args[@]+"${test_args[@]}"} 2>"${workdir}/actual.json" || true
        fi
    else
        "${workdir}/test" ${test_args[@]+"${test_args[@]}"} 2>"${workdir}/actual.json" || true
    fi

    # Check if output is valid JSON
    if ! python3 -c "import json; json.load(open('${workdir}/actual.json'))" 2>/dev/null; then
        echo "FAIL (invalid JSON output)"
        cat "${workdir}/actual.json"
        ((FAILED++))
        rm -f "${instrumented}"
        return 1
    fi

    # Pretty-print, normalize, and save as expected.json. Normalization
    # must match run-golden-tests.sh so the saved file is exactly what
    # the comparison sees - no machine-specific addresses or paths.
    python3 -c "
import json, re
with open('${workdir}/actual.json', 'r') as f:
    data = json.load(f)

def normalize(obj):
    if isinstance(obj, dict):
        return {k: normalize(v) for k, v in sorted(obj.items())}
    if isinstance(obj, list):
        return [normalize(v) for v in obj]
    if isinstance(obj, str):
        obj = re.sub(r'0x[0-9a-fA-F]+', '0xPTR', obj)
        obj = re.sub(r'(?:<ROOT>/|/[^\s\"]*?/)(tests/golden/)', r'<ROOT>/\1', obj)
    return obj

with open('${expected}', 'w') as f:
    json.dump(normalize(data), f, indent=2, sort_keys=True)
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

# Optional filter: when TESTS is set to a space-separated list of test names,
# only those tests are updated. All others are left untouched.
declare -a TEST_FILTER=()
if [[ -n "${TESTS:-}" ]]; then
    read -r -a TEST_FILTER <<< "${TESTS}"
    echo "(filter: ${TEST_FILTER[*]})"
fi

filter_allows() {
    local name="$1"
    if [[ ${#TEST_FILTER[@]} -eq 0 ]]; then
        return 0
    fi
    local f
    for f in "${TEST_FILTER[@]}"; do
        [[ "${f}" == "${name}" ]] && return 0
    done
    return 1
}

# Find all test directories
for test_dir in "${GOLDEN_DIR}"/*; do
    if [[ -d "${test_dir}" ]]; then
        # Check for subdirectories (category/test structure)
        if ls "${test_dir}"/*/input.cpp >/dev/null 2>&1; then
            # Has subdirectories with tests
            for subdir in "${test_dir}"/*; do
                if [[ -d "${subdir}" ]]; then
                    filter_allows "$(basename "${subdir}")" && update_test "${subdir}" || true
                fi
            done
        elif [[ -f "${test_dir}/input.cpp" ]]; then
            # Direct test directory
            filter_allows "$(basename "${test_dir}")" && update_test "${test_dir}" || true
        fi
    fi
done

echo ""
echo "=== Results ==="
echo "Updated: ${UPDATED}"
echo "Failed:  ${FAILED}"
echo "Skipped: ${SKIPPED}"
