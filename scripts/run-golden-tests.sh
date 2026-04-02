#!/usr/bin/env bash
# Run golden tests for C++ Runtime Inspector instrumentation.
#
# For each test directory in tests/golden/:
#   1. Instrument input.cpp using the plugin
#   2. Compile and link with runtime
#   3. Run and capture trace output
#   4. Compare with expected.json (normalized)
#
# Exit 0 if all tests pass, non-zero otherwise.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
GOLDEN_DIR="${ROOT}/tests/golden"
RUNTIME_HDR="${ROOT}/runtime"

# Determine build directory (env var takes priority)
if [[ -n "${BUILD_DIR:-}" ]]; then
    BUILD="${ROOT}/${BUILD_DIR}"
elif [[ -d "${ROOT}/cmake-build-debug" ]]; then
    BUILD="${ROOT}/cmake-build-debug"
else
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
# Malloc shim is optional - only needed for malloc tests
HAVE_MALLOC_SHIM=0
[[ -f "${MALLOC_SHIM}" ]] && HAVE_MALLOC_SHIM=1

# Temporary directory for test outputs
TMPDIR=$(mktemp -d)
trap 'rm -rf "${TMPDIR}"' EXIT

# Counters
PASSED=0
FAILED=0
SKIPPED=0

# Normalize JSON for comparison:
# - Replace addresses with 0xPTR
# - Replace heap IDs with monotonic H1, H2, ...
# - Sort keys for consistent ordering
normalize_json() {
    local input="$1"
    python3 -c "
import json
import re
import sys

def normalize(obj, heap_map=None):
    if heap_map is None:
        heap_map = {'counter': 0}

    if isinstance(obj, dict):
        return {k: normalize(v, heap_map) for k, v in sorted(obj.items())}
    elif isinstance(obj, list):
        return [normalize(v, heap_map) for v in obj]
    elif isinstance(obj, str):
        # Normalize addresses
        obj = re.sub(r'0x[0-9a-fA-F]+', '0xPTR', obj)
        return obj
    else:
        return obj

try:
    with open('$input', 'r') as f:
        data = json.load(f)
    normalized = normalize(data)
    print(json.dumps(normalized, sort_keys=True, indent=2))
except Exception as e:
    print(f'Error: {e}', file=sys.stderr)
    sys.exit(1)
"
}

# Run a single test
run_test() {
    local test_dir="$1"
    local test_name=$(basename "${test_dir}")
    local input="${test_dir}/input.cpp"
    local expected="${test_dir}/expected.json"

    if [[ ! -f "${input}" ]]; then
        echo "SKIP: ${test_name} (no input.cpp)"
        ((SKIPPED++))
        return 0
    fi

    if [[ ! -f "${expected}" ]]; then
        echo "SKIP: ${test_name} (no expected.json)"
        ((SKIPPED++))
        return 0
    fi

    local workdir="${TMPDIR}/${test_name}"
    mkdir -p "${workdir}"

    echo -n "TEST: ${test_name} ... "

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
        return 1
    fi

    # Pass 3: Link
    # On Linux, -rdynamic exports the executable's symbols so an LD_PRELOAD
    # shim (libinspector_malloc_shim.so) can resolve __inspector_alloc_malloc
    # back to the runtime statically linked into the test binary. macOS dyld
    # exports symbols by default, so the flag is unnecessary there.
    LINK_EXTRA=()
    if [[ "$(uname)" != "Darwin" ]]; then
        LINK_EXTRA+=(-rdynamic)
    fi
    if ! "${CLANGXX}" \
        "${workdir}/test.o" \
        "${RUNTIME}" \
        "${LINK_EXTRA[@]}" \
        -o "${workdir}/test" 2>"${workdir}/link.log"; then
        echo "FAIL (linking)"
        cat "${workdir}/link.log"
        ((FAILED++))
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

    if [[ ${use_shim} -eq 1 ]]; then
        # Run with malloc shim
        if [[ "$(uname)" == "Darwin" ]]; then
            DYLD_INSERT_LIBRARIES="${MALLOC_SHIM}" "${workdir}/test" 2>"${workdir}/actual.json" || true
        else
            LD_PRELOAD="${MALLOC_SHIM}" "${workdir}/test" 2>"${workdir}/actual.json" || true
        fi
    else
        if ! "${workdir}/test" 2>"${workdir}/actual.json"; then
            # Non-zero exit is OK for some tests
            :
        fi
    fi

    # Check if output is valid JSON
    if ! python3 -c "import json; json.load(open('${workdir}/actual.json'))" 2>/dev/null; then
        echo "FAIL (invalid JSON output)"
        cat "${workdir}/actual.json"
        ((FAILED++))
        return 1
    fi

    # Normalize and compare
    normalize_json "${workdir}/actual.json" > "${workdir}/actual.normalized.json" || {
        echo "FAIL (normalization error)"
        ((FAILED++))
        return 1
    }

    normalize_json "${expected}" > "${workdir}/expected.normalized.json" || {
        echo "FAIL (expected normalization error)"
        ((FAILED++))
        return 1
    }

    if diff -q "${workdir}/expected.normalized.json" "${workdir}/actual.normalized.json" >/dev/null; then
        echo "PASS"
        ((PASSED++))
        # Clean up instrumented file
        rm -f "${instrumented}"
        return 0
    else
        echo "FAIL (output mismatch)"
        echo "--- Expected (normalized) ---"
        cat "${workdir}/expected.normalized.json"
        echo "--- Actual (normalized) ---"
        cat "${workdir}/actual.normalized.json"
        echo "--- Diff ---"
        diff "${workdir}/expected.normalized.json" "${workdir}/actual.normalized.json" || true
        ((FAILED++))
        return 1
    fi
}

# Main
echo "=== C++ Runtime Inspector Golden Tests ==="
echo ""

# Find all test directories
for test_dir in "${GOLDEN_DIR}"/*; do
    if [[ -d "${test_dir}" ]]; then
        # Check for subdirectories (category/test structure)
        if ls "${test_dir}"/*/input.cpp >/dev/null 2>&1; then
            # Has subdirectories with tests
            for subdir in "${test_dir}"/*; do
                if [[ -d "${subdir}" ]]; then
                    run_test "${subdir}" || true
                fi
            done
        elif [[ -f "${test_dir}/input.cpp" ]]; then
            # Direct test directory
            run_test "${test_dir}" || true
        fi
    fi
done

echo ""
echo "=== Results ==="
echo "Passed:  ${PASSED}"
echo "Failed:  ${FAILED}"
echo "Skipped: ${SKIPPED}"

if [[ ${FAILED} -gt 0 ]]; then
    exit 1
fi

exit 0
