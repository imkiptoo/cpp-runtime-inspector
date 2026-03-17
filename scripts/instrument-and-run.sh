#!/usr/bin/env bash
# Build, instrument, link, and run the example program end-to-end.
#
# A Clang AST plugin's Rewriter modifies a text buffer, not the AST that gets
# lowered to IR by the main compilation. So a source-to-source instrumenter
# needs TWO clang invocations:
#
#   Pass 1: clang -fsyntax-only -fplugin=...  test/example.cpp
#           Plugin walks AST, writes test/example.cpp.instrumented.cpp
#           No object file is produced.
#
#   Pass 2: clang++ -c  test/example.cpp.instrumented.cpp
#           Plain compile of the rewritten source. NO plugin this time.
#           Produces example.o with the __see_* calls present in real codegen.
#
#   Pass 3: clang++ example.o libsee_runtime.a -o example
#           Link against the runtime that defines __see_*.
#
# Run from the repo root:    ./scripts/instrument-and-run.sh

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC="${ROOT}/tests/examples/example.cpp"
RUNTIME_HDR="${ROOT}/runtime"

# Use cmake-build-debug if it exists, otherwise build/
BUILD="${ROOT}/cmake-build-debug"
if [[ ! -d "${BUILD}" ]]; then
    BUILD="${ROOT}/build"
fi
RUNTIME="${BUILD}/libsee_runtime.a"

# Plugin extension varies by platform
if [[ "$(uname)" == "Darwin" ]]; then
    PLUGIN="${BUILD}/libSeePlugin.dylib"
    CLANGXX="${CLANGXX:-/opt/homebrew/opt/llvm/bin/clang++}"
else
    PLUGIN="${BUILD}/libSeePlugin.so"
    CLANGXX="${CLANGXX:-clang++}"
fi

[[ -f "${PLUGIN}"  ]] || { echo "Plugin not built. Run cmake first."  >&2; exit 1; }
[[ -f "${RUNTIME}" ]] || { echo "Runtime not built. Run cmake first." >&2; exit 1; }

# Pass 1: rewrite. -fsyntax-only avoids producing an object file we'd ignore.
# The plugin writes ${SRC}.instrumented.cpp on success.
echo "===== pass 1: instrumenting ====="
"${CLANGXX}" \
    -fsyntax-only \
    -fplugin="${PLUGIN}" \
    -I"${RUNTIME_HDR}" \
    -std=c++17 \
    "${SRC}"

INSTRUMENTED="${SRC}.instrumented.cpp"
[[ -f "${INSTRUMENTED}" ]] || { echo "Plugin did not produce ${INSTRUMENTED}" >&2; exit 1; }

echo
echo "===== rewritten source ====="
cat "${INSTRUMENTED}"
echo

# Pass 2: compile the rewritten source. NO plugin.
echo "===== pass 2: compiling rewritten source ====="
"${CLANGXX}" \
    -I"${RUNTIME_HDR}" \
    -O0 -g \
    -std=c++17 \
    -c "${INSTRUMENTED}" \
    -o "${BUILD}/example.o"

# Pass 3: link.
echo "===== pass 3: linking ====="
"${CLANGXX}" \
    "${BUILD}/example.o" \
    "${RUNTIME}" \
    -o "${BUILD}/example"

# Run. Trace goes to stderr; capture for inspection.
echo
echo "===== running instrumented binary ====="
"${BUILD}/example" 2> "${BUILD}/trace.json"
RC=$?
echo "exit code: ${RC}"
echo
echo "===== trace.json ====="
cat "${BUILD}/trace.json"
echo
echo "(saved to ${BUILD}/trace.json)"
