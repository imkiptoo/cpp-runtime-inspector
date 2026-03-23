#!/bin/bash
#
# C++ Runtime Inspector - Sandbox Execution Script
#
# Reads C++ source from stdin, instruments it, compiles, runs, and emits
# the trace JSON to stdout. Errors go to stderr.
#
# Usage:
#   echo 'int main() { int x = 5; return 0; }' | ./run-traced.sh > trace.json
#
# Environment:
#   INSPECTOR_LIB     - Path to inspector libraries (default: /opt/inspector/lib)
#   INSPECTOR_INCLUDE - Path to inspector headers (default: /opt/inspector/include)
#   TIMEOUT           - Execution timeout in seconds (default: 10)
#   MAX_MEMORY        - Memory limit in KB (default: 262144 = 256MB)

set -e

# Configuration
INSPECTOR_LIB="${INSPECTOR_LIB:-/opt/inspector/lib}"
INSPECTOR_INCLUDE="${INSPECTOR_INCLUDE:-/opt/inspector/include}"
TIMEOUT="${TIMEOUT:-10}"
MAX_MEMORY="${MAX_MEMORY:-262144}"

# Temporary files
WORKDIR=$(mktemp -d)
trap "rm -rf '$WORKDIR'" EXIT

INPUT_FILE="$WORKDIR/input.cpp"
INSTRUMENTED_FILE="$WORKDIR/input.cpp.instrumented.cpp"
OUTPUT_BINARY="$WORKDIR/program"
TRACE_FILE="$WORKDIR/trace.json"

# Read source from stdin
cat > "$INPUT_FILE"

# Check if input is empty
if [ ! -s "$INPUT_FILE" ]; then
    echo '{"error": "empty_input", "message": "No C++ source code provided"}' >&2
    exit 1
fi

# Step 1: Instrument the source
echo "Instrumenting source..." >&2
if ! clang++ -std=c++17 -fsyntax-only \
    -fplugin="$INSPECTOR_LIB/libInspectorPlugin.so" \
    -I"$INSPECTOR_INCLUDE" \
    "$INPUT_FILE" 2>&1; then
    echo '{"error": "instrumentation_failed", "message": "Clang plugin failed to instrument source"}' >&2
    exit 2
fi

# Check if instrumented file was created
if [ ! -f "$INSTRUMENTED_FILE" ]; then
    echo '{"error": "no_instrumented_file", "message": "Plugin did not produce instrumented file"}' >&2
    exit 3
fi

# Step 2: Compile the instrumented source
echo "Compiling instrumented code..." >&2
if ! clang++ -std=c++17 -O0 -g \
    -I"$INSPECTOR_INCLUDE" \
    "$INSTRUMENTED_FILE" \
    "$INSPECTOR_LIB/libinspector_runtime.a" \
    -o "$OUTPUT_BINARY" 2>&1; then
    echo '{"error": "compilation_failed", "message": "Failed to compile instrumented source"}' >&2
    exit 4
fi

# Step 3: Run with resource limits
echo "Running program..." >&2

# Set resource limits and run
ulimit -v "$MAX_MEMORY" 2>/dev/null || true
ulimit -t "$TIMEOUT" 2>/dev/null || true

# Run and capture trace from stderr
if timeout "$TIMEOUT"s "$OUTPUT_BINARY" 2> "$TRACE_FILE"; then
    # Program succeeded
    EXIT_CODE=0
else
    EXIT_CODE=$?
    if [ "$EXIT_CODE" -eq 124 ]; then
        echo '{"error": "timeout", "message": "Program exceeded time limit"}' >&2
        # Try to emit partial trace if available
        if [ -s "$TRACE_FILE" ]; then
            cat "$TRACE_FILE"
        fi
        exit 5
    fi
fi

# Emit the trace
if [ -s "$TRACE_FILE" ]; then
    cat "$TRACE_FILE"
else
    echo '{"error": "no_trace", "message": "Program did not produce trace output"}'
fi

exit "$EXIT_CODE"
