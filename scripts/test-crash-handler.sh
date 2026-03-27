#!/usr/bin/env bash
# Test crash signal handlers.
#
# This script tests that the runtime properly handles crashes and emits
# crash information before terminating.

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${ROOT}/cmake-build-debug"
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

WORKDIR=$(mktemp -d)
trap 'rm -rf "${WORKDIR}"' EXIT

echo "=== Crash Handler Tests ==="
echo ""

# Test 1: SIGSEGV (null pointer dereference)
echo "Test 1: SIGSEGV (null pointer dereference)..."

cat > "${WORKDIR}/crash_segv.cpp" << 'EOF'
int main() {
    int x = 42;
    int* ptr = nullptr;
    int crash = *ptr;  // This will crash
    return 0;
}
EOF

# Instrument
"${CLANGXX}" \
    -fsyntax-only \
    -fplugin="${PLUGIN}" \
    -I"${RUNTIME_HDR}" \
    -std=c++17 \
    "${WORKDIR}/crash_segv.cpp" 2>/dev/null

# Compile
"${CLANGXX}" \
    -I"${RUNTIME_HDR}" \
    -O0 -g \
    -std=c++17 \
    -c "${WORKDIR}/crash_segv.cpp.instrumented.cpp" \
    -o "${WORKDIR}/crash_segv.o"

# Link
"${CLANGXX}" \
    "${WORKDIR}/crash_segv.o" \
    "${RUNTIME}" \
    -o "${WORKDIR}/crash_segv"

# Run and capture output (will crash)
"${WORKDIR}/crash_segv" 2>"${WORKDIR}/crash_segv.output" || true

# Check output contains crash info
if grep -q '"crash"' "${WORKDIR}/crash_segv.output"; then
    echo "  PASS: Crash JSON emitted"
else
    echo "  FAIL: No crash JSON found"
    cat "${WORKDIR}/crash_segv.output"
fi

if grep -q '"SIGSEGV"' "${WORKDIR}/crash_segv.output"; then
    echo "  PASS: Signal name included"
else
    echo "  FAIL: Signal name missing"
fi

# Check that some trace was emitted before the crash
if grep -q '"trace"' "${WORKDIR}/crash_segv.output" || grep -q '"x": 42' "${WORKDIR}/crash_segv.output"; then
    echo "  PASS: Partial trace emitted before crash"
else
    echo "  INFO: No partial trace (may depend on crash timing)"
fi

rm -f "${WORKDIR}/crash_segv.cpp.instrumented.cpp"

# Test 2: SIGABRT (abort)
echo ""
echo "Test 2: SIGABRT (abort)..."

cat > "${WORKDIR}/crash_abort.cpp" << 'EOF'
#include <cstdlib>

int main() {
    int x = 100;
    std::abort();  // This will abort
    return 0;
}
EOF

# Instrument
"${CLANGXX}" \
    -fsyntax-only \
    -fplugin="${PLUGIN}" \
    -I"${RUNTIME_HDR}" \
    -std=c++17 \
    "${WORKDIR}/crash_abort.cpp" 2>/dev/null

# Compile
"${CLANGXX}" \
    -I"${RUNTIME_HDR}" \
    -O0 -g \
    -std=c++17 \
    -c "${WORKDIR}/crash_abort.cpp.instrumented.cpp" \
    -o "${WORKDIR}/crash_abort.o"

# Link
"${CLANGXX}" \
    "${WORKDIR}/crash_abort.o" \
    "${RUNTIME}" \
    -o "${WORKDIR}/crash_abort"

# Run and capture output (will abort)
"${WORKDIR}/crash_abort" 2>"${WORKDIR}/crash_abort.output" || true

# Check output contains crash info
if grep -q '"crash"' "${WORKDIR}/crash_abort.output"; then
    echo "  PASS: Crash JSON emitted"
else
    echo "  FAIL: No crash JSON found"
    cat "${WORKDIR}/crash_abort.output"
fi

if grep -q '"SIGABRT"' "${WORKDIR}/crash_abort.output"; then
    echo "  PASS: Signal name included"
else
    echo "  FAIL: Signal name missing"
fi

rm -f "${WORKDIR}/crash_abort.cpp.instrumented.cpp"

# Test 3: SIGFPE (division by zero)
echo ""
echo "Test 3: SIGFPE (division by zero)..."

cat > "${WORKDIR}/crash_fpe.cpp" << 'EOF'
int main() {
    int x = 10;
    int y = 0;
    int z = x / y;  // This will cause FPE
    return z;
}
EOF

# Instrument
"${CLANGXX}" \
    -fsyntax-only \
    -fplugin="${PLUGIN}" \
    -I"${RUNTIME_HDR}" \
    -std=c++17 \
    "${WORKDIR}/crash_fpe.cpp" 2>/dev/null

# Compile (need to prevent compiler from optimizing out the division)
"${CLANGXX}" \
    -I"${RUNTIME_HDR}" \
    -O0 -g \
    -std=c++17 \
    -c "${WORKDIR}/crash_fpe.cpp.instrumented.cpp" \
    -o "${WORKDIR}/crash_fpe.o"

# Link
"${CLANGXX}" \
    "${WORKDIR}/crash_fpe.o" \
    "${RUNTIME}" \
    -o "${WORKDIR}/crash_fpe"

# Run and capture output (will crash)
"${WORKDIR}/crash_fpe" 2>"${WORKDIR}/crash_fpe.output" || true

# Check output
if grep -q '"crash"' "${WORKDIR}/crash_fpe.output"; then
    echo "  PASS: Crash JSON emitted"
elif grep -q '"trace"' "${WORKDIR}/crash_fpe.output"; then
    # Some platforms don't trap integer division by zero
    echo "  INFO: No crash on this platform (integer division by zero not trapped)"
else
    echo "  FAIL: No crash or trace output"
    cat "${WORKDIR}/crash_fpe.output"
fi

rm -f "${WORKDIR}/crash_fpe.cpp.instrumented.cpp"

echo ""
echo "=== Crash handler tests complete ==="
