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

# Parse command line arguments
SINGLE_TEST=""
VERBOSE=0
INSTRUMENT_ONLY=0
while [[ $# -gt 0 ]]; do
    case $1 in
        -t|--test)
            SINGLE_TEST="$2"
            shift 2
            ;;
        -v|--verbose)
            VERBOSE=1
            shift
            ;;
        -i|--instrument-only)
            INSTRUMENT_ONLY=1
            VERBOSE=1  # Automatically enable verbose to show the output
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  -t, --test NAME       Run only the specified test"
            echo "  -v, --verbose         Show more output (instrumented code)"
            echo "  -i, --instrument-only Just instrument, don't compile or run"
            echo "  -h, --help            Show this help"
            echo ""
            echo "Examples:"
            echo "  $0                              # Run all tests"
            echo "  $0 -t new_struct_ptr            # Run only new_struct_ptr test"
            echo "  $0 -t heap_basic -v             # Run heap_basic with verbose output"
            echo "  $0 -t new_struct_ptr -i         # Just show instrumented code"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

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

def normalize_string(s):
    # Normalize addresses
    s = re.sub(r'0x[0-9a-fA-F]+', '0xPTR', s)
    # Normalize absolute paths to anything under tests/golden/ so that
    # checkout location does not affect the comparison. Lambda type
    # strings, for example, embed the full source path.
    s = re.sub(r'(?:<ROOT>/|/[^\s\"]*?/)(tests/golden/)', r'<ROOT>/\1', s)
    return s

def normalize(obj, heap_map=None, parent_key=None):
    if heap_map is None:
        heap_map = {'counter': 0}

    if isinstance(obj, dict):
        # Normalize both keys and values
        return {normalize_string(k): normalize(v, heap_map, k) for k, v in sorted(obj.items())}
    elif isinstance(obj, list):
        normalized = [normalize(v, heap_map) for v in obj]
        # Sort memory_leaks array for deterministic comparison
        if parent_key == 'memory_leaks':
            normalized = sorted(normalized, key=lambda x: str(x))
        return normalized
    elif isinstance(obj, str):
        return normalize_string(obj)
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

    # Use platform-specific expected file if available
    if [[ "$(uname)" == "Darwin" && -f "${test_dir}/expected.darwin.json" ]]; then
        expected="${test_dir}/expected.darwin.json"
    elif [[ "$(uname)" == "Linux" && -f "${test_dir}/expected.linux.json" ]]; then
        expected="${test_dir}/expected.linux.json"
    fi

    if [[ ! -f "${expected}" && ${INSTRUMENT_ONLY} -eq 0 ]]; then
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

    # Show instrumented code in verbose mode
    if [[ ${VERBOSE} -eq 1 ]]; then
        echo ""
        echo "--- Instrumented code ---"
        cat "${instrumented}"
        echo "--- End instrumented code ---"
        echo ""
    fi

    # Stop here if instrument-only mode
    if [[ ${INSTRUMENT_ONLY} -eq 1 ]]; then
        echo "DONE (instrument only)"
        ((PASSED++))
        return 0
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
    # Both platforms need to export the runtime's symbols so that:
    # 1. The malloc shim can resolve __inspector_alloc_malloc back to
    #    the runtime statically linked into the test binary.
    # 2. The dynamic-type resolver (Dynamic.cpp) can map vtable pointers
    #    back to symbol names via dladdr.
    LINK_EXTRA=()
    if [[ "$(uname)" == "Darwin" ]]; then
        # macOS: -export_dynamic exports all symbols from the executable.
        LINK_EXTRA+=(-Wl,-export_dynamic)
    else
        # Linux: -rdynamic does the same thing, -ldl links the dynamic-loader API.
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
        # Skip shim tests on Linux CI - LD_PRELOAD doesn't work reliably there
        if [[ "$(uname)" == "Linux" && -n "${CI:-}" ]]; then
            echo "SKIP (shim tests disabled on Linux CI)"
            ((SKIPPED++))
            rm -f "${instrumented}"
            return 0
        fi
    fi

    # Optional CLI args: a .args file in the test dir is split on whitespace
    # and passed to the binary. Used by argc_argv to produce deterministic
    # output without depending on the runner's tmpdir path.
    local -a test_args=()
    if [[ -f "${test_dir}/.args" ]]; then
        read -r -a test_args < "${test_dir}/.args"
    fi

    # Optional stdin input: a .stdin file provides input to the program
    local stdin_file="/dev/null"
    if [[ -f "${test_dir}/.stdin" ]]; then
        stdin_file="${test_dir}/.stdin"
    fi

    # Set timeout (30 seconds should be plenty for any test)
    local timeout_prefix=""
    if command -v timeout &>/dev/null; then
        timeout_prefix="timeout 30"
    fi

    if [[ ${use_shim} -eq 1 ]]; then
        # Run with malloc shim - use env to set LD_PRELOAD only for the test binary
        if [[ "$(uname)" == "Darwin" ]]; then
            DYLD_INSERT_LIBRARIES="${MALLOC_SHIM}" ${timeout_prefix} "${workdir}/test" ${test_args[@]+"${test_args[@]}"} <"${stdin_file}" 2>"${workdir}/actual.json" || true
        else
            ${timeout_prefix} env LD_PRELOAD="${MALLOC_SHIM}" "${workdir}/test" ${test_args[@]+"${test_args[@]}"} <"${stdin_file}" 2>"${workdir}/actual.json" || true
        fi
    else
        if ! ${timeout_prefix} "${workdir}/test" ${test_args[@]+"${test_args[@]}"} <"${stdin_file}" 2>"${workdir}/actual.json"; then
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

    # For shim tests, use lenient comparison that ignores heap contents
    # (internal library allocations vary by platform/run)
    local actual_cmp="${workdir}/actual.normalized.json"
    local expected_cmp="${workdir}/expected.normalized.json"
    if [[ ${use_shim} -eq 1 ]]; then
        python3 -c "
import json, sys

def normalize_for_shim(obj, id_map=None):
    '''Strip heap/memory_leaks/heap_sizes and normalize heap IDs in REF/DANGLING'''
    if id_map is None:
        id_map = {'counter': 0}
    if isinstance(obj, dict):
        # Strip heap, memory_leaks, heap_sizes, heap_total_bytes (internal allocations vary)
        return {k: normalize_for_shim(v, id_map) for k, v in obj.items()
                if k not in ('heap', 'memory_leaks', 'heap_sizes', 'heap_total_bytes', 'heap_addresses')}
    if isinstance(obj, list):
        # Check for REF/DANGLING patterns: ['REF', id] or ['DANGLING', id]
        if len(obj) == 2 and obj[0] in ('REF', 'DANGLING') and isinstance(obj[1], int):
            old_id = obj[1]
            if old_id not in id_map:
                id_map['counter'] += 1
                id_map[old_id] = id_map['counter']
            return [obj[0], id_map[old_id]]
        return [normalize_for_shim(v, id_map) for v in obj]
    return obj

with open(sys.argv[1]) as f:
    d = json.load(f)
had_leaks = 'memory_leaks' in d and len(d.get('memory_leaks', [])) > 0
print(json.dumps({'data': normalize_for_shim(d), 'had_leaks': had_leaks}, sort_keys=True, indent=2))
" "${workdir}/actual.normalized.json" > "${workdir}/actual.shim.json"
        python3 -c "
import json, sys

def normalize_for_shim(obj, id_map=None):
    '''Strip heap/memory_leaks/heap_sizes and normalize heap IDs in REF/DANGLING'''
    if id_map is None:
        id_map = {'counter': 0}
    if isinstance(obj, dict):
        # Strip heap, memory_leaks, heap_sizes, heap_total_bytes (internal allocations vary)
        return {k: normalize_for_shim(v, id_map) for k, v in obj.items()
                if k not in ('heap', 'memory_leaks', 'heap_sizes', 'heap_total_bytes', 'heap_addresses')}
    if isinstance(obj, list):
        if len(obj) == 2 and obj[0] in ('REF', 'DANGLING') and isinstance(obj[1], int):
            old_id = obj[1]
            if old_id not in id_map:
                id_map['counter'] += 1
                id_map[old_id] = id_map['counter']
            return [obj[0], id_map[old_id]]
        return [normalize_for_shim(v, id_map) for v in obj]
    return obj

with open(sys.argv[1]) as f:
    d = json.load(f)
had_leaks = 'memory_leaks' in d and len(d.get('memory_leaks', [])) > 0
print(json.dumps({'data': normalize_for_shim(d), 'had_leaks': had_leaks}, sort_keys=True, indent=2))
" "${workdir}/expected.normalized.json" > "${workdir}/expected.shim.json"
        actual_cmp="${workdir}/actual.shim.json"
        expected_cmp="${workdir}/expected.shim.json"
    fi

    if diff -q "${expected_cmp}" "${actual_cmp}" >/dev/null; then
        echo "PASS"
        ((PASSED++))
        # Clean up instrumented file
        rm -f "${instrumented}"
        return 0
    else
        echo "FAIL (output mismatch)"
        echo "--- Expected (normalized) ---"
        cat "${expected_cmp}"
        echo "--- Actual (normalized) ---"
        cat "${actual_cmp}"
        echo "--- Diff ---"
        diff "${expected_cmp}" "${actual_cmp}" || true
        ((FAILED++))
        return 1
    fi
}

# Main
echo "=== C++ Runtime Inspector Golden Tests ==="
echo ""

# If a single test is specified, run only that
if [[ -n "${SINGLE_TEST}" ]]; then
    test_dir="${GOLDEN_DIR}/${SINGLE_TEST}"
    if [[ -d "${test_dir}" && -f "${test_dir}/input.cpp" ]]; then
        echo "Running single test: ${SINGLE_TEST}"
        echo ""
        run_test "${test_dir}" || true
    else
        echo "Error: Test '${SINGLE_TEST}' not found at ${test_dir}"
        echo ""
        echo "Available tests:"
        for d in "${GOLDEN_DIR}"/*; do
            if [[ -d "$d" && -f "$d/input.cpp" ]]; then
                echo "  $(basename "$d")"
            fi
        done
        exit 1
    fi
else
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
fi

echo ""
echo "=== Results ==="
echo "Passed:  ${PASSED}"
echo "Failed:  ${FAILED}"
echo "Skipped: ${SKIPPED}"

if [[ ${FAILED} -gt 0 ]]; then
    exit 1
fi

exit 0
