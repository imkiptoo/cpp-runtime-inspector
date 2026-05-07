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

The runtime library (`core/runtime/inspector_runtime.*`) collects trace data:
- Maintains a call stack with local variables
- Serializes state changes to JSON format
- Writes to stderr (so stdout is available for program output)

## Architecture

### Components

| Component | Purpose | Size |
|-----------|---------|------|
| `core/plugin/InspectorPlugin.cpp` | Clang AST plugin that rewrites source | ~300 lines |
| `core/plugin/Visitor.cpp` | AST node visitor (functions, variables, statements) | ~150 lines |
| `core/plugin/TypeEncoder.cpp` | Type information extraction | ~100 lines |
| `core/runtime/inspector_runtime.cpp` | Tracing runtime (call stack, JSON emit) | ~250 lines |
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
- **All primitive types** (int, float, double, bool, char)
- **Pointers and references** (with stack/heap region tracking)
- **Heap allocations** (new/delete, malloc/free via shim)
- **User-defined types** (structs, classes, enums, unions)
- **STL containers** (vector, string, unique_ptr, shared_ptr, optional)
- **Templates** (function and class templates)
- **Compound assignments** (+=, -=, ++, --, etc.)
- **Constructors/Destructors** (including copy/move)
- **Virtual dispatch** (dynamic type tracking)
- **Lambda expressions** (with capture tracking)
- **Exception handling** (throw/catch events)

Partially supported:
- **std::map/set** (placeholder - tree traversal limited)
- **Multiple inheritance** (warning emitted, skipped)

## Known Limitations

These are documented implementation gaps, not design flaws:

### 1. Multiple Inheritance
Multiple inheritance and virtual inheritance are not supported. The plugin emits a warning and skips instrumentation for such types.

### 2. Threading
Multi-threaded programs (`std::thread`, `std::async`) are not supported. A warning is emitted. The tracer assumes single-threaded execution.

### 3. Optimization Levels
Programs must be compiled with `-O0`. Higher optimization levels may produce incorrect traces due to inlining, dead code elimination, etc.

### 4. Complex STL Containers
`std::map` and `std::set` have limited support (tree traversal placeholder). Hash containers (`std::unordered_*`) are not supported.

### 5. Platform Restrictions
- **Linux x86_64**: Fully supported (libstdc++)
- **macOS**: Supported with limitations (libc++ differences)
- **Windows**: Not supported (no MSVC)

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
│
├── core/                          # C++ components
│   ├── plugin/                    # Clang plugin source
│   │   ├── InspectorPlugin.cpp    # Main plugin entry point
│   │   ├── Visitor.cpp            # AST visitor implementation
│   │   ├── TypeEncoder.cpp        # Type information extraction
│   │   └── ...
│   ├── runtime/                   # Runtime library (linked into instrumented binaries)
│   │   ├── inspector_runtime.cpp
│   │   ├── inspector/
│   │   │   ├── Trace.cpp          # Trace data collection
│   │   │   ├── JsonEmit.cpp       # JSON serialization
│   │   │   └── ...
│   └── shim/                      # malloc/free interception shim
│       └── inspector_malloc_shim.c
│
├── services/
│   ├── api/                       # HTTP server (Python/Flask)
│   │   ├── server.py
│   │   └── Dockerfile
│   └── sandbox/                   # Docker-based execution sandbox
│       ├── Dockerfile
│       ├── docker-compose.yml
│       └── run-traced.sh
│
├── web/                           # SvelteKit frontend
│   ├── src/
│   ├── package.json
│   └── Dockerfile.prod
│
├── deploy/                        # Deployment configs
│   ├── Dockerfile.server
│   └── docker-compose.yml
│
├── tests/
│   ├── golden/                    # End-to-end test cases
│   │   ├── primitives/
│   │   ├── heap_basic/
│   │   └── ...
│   └── tier2_test.cpp             # Unit tests
│
├── scripts/
│   ├── instrument-and-run.sh      # Build driver
│   └── run-golden-tests.sh        # Test runner
│
└── docs/                          # Documentation
    ├── architecture.md
    ├── trace-format.md
    └── ...
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

## Web Interface

The project includes a SvelteKit-based visualization frontend in `web/`:

```bash
# Development
cd web && npm install && npm run dev

# Production (Docker)
docker compose -f deploy/docker-compose.yml up --build
```

Features:
- Interactive code editor with C++ syntax highlighting
- Step-by-step execution visualization
- Call stack and variable state display
- Heap memory visualization with pointer tracking
- Memory leak detection display

## License

This is a proof-of-concept and research artifact. Modify and build upon as needed.

## References

- [LLVM Clang Plugin Documentation](https://clang.llvm.org/docs/Plugins.html)
- [OPT Trace Format](https://github.com/pgbovine/OnlinePythonTutor/blob/master/v3/docs/opt-trace-format.md) (Python Tutor standard)
- [nlohmann/json](https://github.com/nlohmann/json) — JSON library used by runtime
