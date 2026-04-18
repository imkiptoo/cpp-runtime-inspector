# C++ Runtime Inspector

A proof-of-concept tool that automatically instruments C++ programs to trace their execution at runtime. It uses Clang AST plugins to rewrite source code, injecting calls to capture function entries/exits, variable initialization, and state changes. The result is a working binary that emits a JSON execution trace compatible with visualization tools like Python Tutor.

## Quick Start

### Prerequisites

**macOS (Homebrew):**
```bash
brew install llvm cmake ninja
```

**Ubuntu 22.04+:**
```bash
sudo apt install llvm-dev libclang-dev clang cmake ninja-build
```

### Build

```bash
cmake -B cmake-build-debug -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DLLVM_DIR=/opt/homebrew/opt/llvm/lib/cmake/llvm \
  -DClang_DIR=/opt/homebrew/opt/llvm/lib/cmake/clang
cmake --build cmake-build-debug -j$(nproc)
```

### Run

```bash
./scripts/instrument-and-run.sh
```

This script:
1. Instruments `test/example.cpp` using the plugin
2. Compiles the instrumented source
3. Runs the binary and captures the JSON trace to `trace.json`

Example output (10-step trace of a simple program):
```json
[
  { "type": "call",       "name": "main",   "stack": [] },
  { "type": "var_init",   "name": "a",     "stack": [{"name": "a", "value": 3}] },
  { "type": "var_init",   "name": "b",     "stack": [{"name": "a", "value": 3}, {"name": "b", "value": 4}] },
  { "type": "var_update", "name": "sum",   "stack": [...] },
  ...
]
```

## How It Works

The tool operates in three stages:

### Stage 1: AST Analysis & Rewriting (Plugin Pass)
```
clang -fsyntax-only -fplugin=libInspectorPlugin.so user.cpp
```
- Clang loads the plugin and walks the AST
- Plugin identifies function definitions, variable declarations, assignments, returns
- Uses `clang::Rewriter` to inject `__inspector_*` function calls at strategic points
- Writes the rewritten source to `user.cpp.instrumented.cpp`

**Example transformation:**
```cpp
// Before
int add(int a, int b) {
  int result = a + b;
  return result;
}

// After (rewritten by plugin)
int add(int a, int b) {
  __inspector_enter("add");
  int result;
  __inspector_var_init("result");
  result = a + b;
  __inspector_var_update("result", result);
  __inspector_leave("add");
  return result;
}
```

### Stage 2: Compilation
```
clang++ -c user.cpp.instrumented.cpp -o user.o
```
- Standard C++ compilation of the instrumented source

### Stage 3: Linking & Runtime
```
clang++ user.o libinspector_runtime.a -o user
./user 2> trace.json
```
- Link the compiled object with the tracing runtime library
- Run the binary; instrumentation calls write trace events to stderr as JSON

The runtime library (`runtime/inspector_runtime.*`) collects trace data:
- Maintains a call stack with local variables
- Serializes state changes to JSON format
- Writes to stderr (so stdout is available for program output)

## Architecture

### Components

| Component | Purpose | Size |
|-----------|---------|------|
| `plugin/InspectorPlugin.cpp` | Clang AST plugin that rewrites source | ~300 lines |
| `plugin/Visitor.cpp` | AST node visitor (functions, variables, statements) | ~150 lines |
| `plugin/TypeEncoder.cpp` | Type information extraction | ~100 lines |
| `runtime/inspector_runtime.cpp` | Tracing runtime (call stack, JSON emit) | ~250 lines |
| `scripts/instrument-and-run.sh` | Three-stage build driver | ~50 lines |

### Why Three Stages?

The Clang `Rewriter` API works on **source text**, not the AST used by codegen. Therefore:
- A single `clang -fsyntax-only` pass produces instrumented source but won't compile it
- The instrumented source must be compiled in a separate pass
- This two-pass model is standard for source-rewriting tools (clang-tidy, linters, etc.)

**Future optimization:** A single-pass compiler-integration via Clang libtooling or custom IR passes could eliminate the intermediate file, but would require significantly more code.

## What It Traces

Currently tracked:

- **Function calls & returns** (with parameter/return values)
- **Variable initialization** (declaration with initial value)
- **Variable updates** (assignment to existing variable)
- **Call stack state** (locals visible at each step)
- **Integer primitives** (int, long, short)

Not tracked (known limitations below):

- ❌ Floating-point, char, bool, other primitives
- ❌ Pointers and heap allocations
- ❌ User-defined types (structs, classes)
- ❌ STL containers (vector, map, string literal content)
- ❌ Templates (instrumented at definition, not instantiation)
- ❌ Compound assignments (+=, ++, -=, etc.)

## Known Limitations

These are documented implementation gaps, not design flaws:

### 1. Dead Code After Returns
The plugin injects `__inspector_leave()` both before each `return` statement and at the function's closing brace. For functions with explicit returns, the closing-brace leave is unreachable and dead code.

**Fix:** Track-per-function whether the function "falls off" the end (no explicit return), and only emit closing-brace leave in that case.

### 2. Missing Line Numbers
Line information is disabled (`line: 0` in all events) because naive line injection breaks compound expressions:
```cpp
if (cond) stmt;        // becomes if (cond) __step(5); stmt;
```
This is grammatically invalid.

**Fix:** Parent-aware visitor that only injects line events at statements that are direct children of a `CompoundStmt`.

### 3. Limited Type Support
Only `int` is converted to JSON. Other primitives need individual runtime hooks. User-defined types need recursive field encoding.

**Fix:** Add hooks for `long`, `double`, `float`, `bool`; add recursive struct encoding.

### 4. No Heap Tracking
`new` and `delete` aren't intercepted. Useful for memory safety analysis but requires interval-tree-based allocation tracking.

**Fix:** Intercept `CXXNewExpr`/`CXXDeleteExpr`, maintain live allocation intervals at runtime.

### 5. Template Instantiation
Templates are instrumented at definition time, not per instantiation. `std::vector<MyType>` won't be traced per instantiation.

**Fix:** Dual instrumentation (AST for names, IR pass for events).

### 6. Incomplete Operators
Only `=` (assignment) is tracked. Compound `-=`, `+=`, `++`, `--` are not instrumented.

**Fix:** Visit `BinaryOperator` and `UnaryOperator` nodes with all relevant opcodes.

### 7. No Type Encoding
Traces emit raw integer values. The OPT format (Python Tutor's standard) uses type tags: `["REF", heap_id]` for pointers, structured records for user types.

**Fix:** Add a type-tag system and encode complex types as structured records.

## Golden Tests

The project includes end-to-end tests in `tests/golden/`:

```bash
# Run all golden tests
cmake --build cmake-build-debug --target golden-tests

# Example test: primitives
tests/golden/primitives/
  ├── input.cpp          # Source to instrument
  └── expected.json      # Expected trace output
```

Each test has an input C++ file and an expected JSON trace. The build system instruments and runs each input, comparing output to expected.

## Trace Format

The output is a simplified version of the OPT trace format (used by Python Tutor and other visualization tools):

```json
[
  { "type": "call",       "name": "func_name", "stack": [] },
  { "type": "var_init",   "name": "var_name",  "stack": [...] },
  { "type": "var_update", "name": "var_name",  "stack": [...] },
  { "type": "return",     "name": "func_name", "stack": [...] }
]
```

Each stack entry is:
```json
{ "name": "variable_name", "value": value_as_int, "line": 0 }
```

(Line numbers are 0 due to limitation #2 above.)

## Project Organization

```
cpp-runtime-inspector/
├── CMakeLists.txt                 # Build configuration
├── CMakePresets.json              # IDE presets with LLVM/Clang paths
├── plugin/                        # Clang plugin source
│   ├── InspectorPlugin.cpp        # Main plugin entry point
│   ├── Visitor.cpp                # AST visitor implementation
│   ├── TypeEncoder.cpp            # Type information extraction
│   └── ...
├── runtime/                       # Runtime library (linked into instrumented binaries)
│   ├── inspector_runtime.cpp
│   ├── inspector/
│   │   ├── Trace.cpp              # Trace data collection
│   │   ├── JsonEmit.cpp           # JSON serialization
│   │   ├── Heap.cpp               # (Unused in current version)
│   │   └── ...
├── tests/
│   ├── golden/                    # End-to-end test cases
│   │   ├── primitives/
│   │   ├── heap_basic/
│   │   └── ...
│   ├── tier2_test.cpp             # Unit tests
└── scripts/
    ├── instrument-and-run.sh      # Build driver
    └── run-golden-tests.sh        # Test runner
```

## Building for Different LLVM Versions

The build system auto-detects Homebrew LLVM on macOS and system LLVM on Linux. To use a specific version:

**Homebrew LLVM 18:**
```bash
brew install llvm@18
cmake -B build \
  -DLLVM_DIR=/opt/homebrew/opt/llvm@18/lib/cmake/llvm \
  -DClang_DIR=/opt/homebrew/opt/llvm@18/lib/cmake/clang
```

**System LLVM (Ubuntu):**
```bash
sudo apt install llvm-16-dev libclang-16-dev
cmake -B build
```

Note: The plugin must be built against the **exact** LLVM/Clang version you'll use to compile user programs. Mismatches cause crashes in the plugin loader.

## Next Steps

Suggested improvements in rough order of impact:

1. **Fix dead-leave bug** (~30 min) — Remove unreachable `__inspector_leave` calls.
2. **Implement parent-aware line injection** (~2 hrs) — Add real line numbers to trace.
3. **Support more primitive types** (~1 hr) — Add hooks for `double`, `float`, `bool`, `char`.
4. **Struct/class recursion** (~4 hrs) — Encode user-defined types as nested records.
5. **Heap allocation tracking** (~1 day) — Intercept `new`/`delete`, maintain allocation intervals.
6. **Pointer-to-heap resolution** (~1 day) — Lookup heap object by address in trace.
7. **Template instantiation** (~3 days) — IR pass for template-specific instrumentation.
8. **STL support** (~1 week) — Custom matchers for `std::vector`, `std::string`, `std::map`.
9. **Sandboxed runner** (~2 days) — Docker/seccomp/timeout wrapper for safety.

## License

This is a proof-of-concept and research artifact. Modify and build upon as needed.

## References

- [LLVM Clang Plugin Documentation](https://clang.llvm.org/docs/Plugins.html)
- [OPT Trace Format](https://github.com/pgbovine/OnlinePythonTutor/blob/master/v3/docs/opt-trace-format.md) (Python Tutor standard)
- [nlohmann/json](https://github.com/nlohmann/json) — JSON library used by runtime
