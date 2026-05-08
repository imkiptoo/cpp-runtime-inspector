# C++ Runtime Inspector

A C++ instrumentation tool that uses [Clang](https://clang.llvm.org/) [AST](https://clang.llvm.org/docs/IntroductionToTheClangAST.html) [plugins](https://clang.llvm.org/docs/Plugins.html) to automatically inject tracing hooks into source code. The instrumented binary emits a JSON execution trace showing function calls, variable state, and memory allocations—compatible with [Python Tutor](https://pythontutor.com/) and similar visualizers.

**In plain English:** You give it your C++ code, and it shows you exactly what happens when the program runs—step by step. You can see variables change, watch functions get called, and track memory being allocated and freed. It's like a slow-motion replay of your program's execution.

[![Demo](https://img.youtube.com/vi/5eNisAEtXbU/maxresdefault.jpg)](https://www.youtube.com/watch?v=5eNisAEtXbU)

**Documentation:**
[Architecture](docs/architecture.md) ·
[Trace Format](docs/trace-format.md) ·
[Frontend Integration](docs/frontend-integration.md) ·
[Extending STL Encoders](docs/extending-stl-encoders.md) ·
[Supported Language Subset](docs/supported-language-subset.md)

## Quick Start

### Prerequisites

- [LLVM/Clang](https://llvm.org/) 17+ (compiler infrastructure)
- [CMake](https://cmake.org/) 3.20+ (build system)
- [Ninja](https://ninja-build.org/) (recommended build tool)

**macOS (Homebrew):**
```bash
brew install llvm cmake ninja
```

**Ubuntu 22.04+:**
```bash
sudo apt install llvm-dev libclang-dev clang cmake ninja-build
```

### Build

**macOS (Homebrew LLVM):**
```bash
cmake -B cmake-build-debug -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DLLVM_DIR=/opt/homebrew/opt/llvm/lib/cmake/llvm \
  -DClang_DIR=/opt/homebrew/opt/llvm/lib/cmake/clang
cmake --build cmake-build-debug -j$(nproc)
```

**Linux (System LLVM):**
```bash
cmake -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
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

Example trace output:
```json
{
  "code": "int main() { int a = 3; int b = 4; return a + b; }",
  "trace": [
    { "event": "call", "func_name": "main", "line": 1, "stack_to_render": [...] },
    { "event": "step_line", "func_name": "main", "line": 1, "stack_to_render": [{"encoded_locals": {"a": 3}}] },
    { "event": "step_line", "func_name": "main", "line": 1, "stack_to_render": [{"encoded_locals": {"a": 3, "b": 4}}] },
    { "event": "return", "func_name": "main", "line": 1, "stack_to_render": [...] }
  ]
}
```

## How It Works

The tool operates in three stages:

### Stage 1: AST Analysis & Rewriting (Plugin Pass)
```
clang -fsyntax-only -fplugin=libInspectorPlugin.so user.cpp
```
- Clang loads the plugin and walks the [AST](https://clang.llvm.org/docs/IntroductionToTheClangAST.html)
- Plugin identifies function definitions, variable declarations, assignments, returns
- Uses [`clang::Rewriter`](https://clang.llvm.org/doxygen/classclang_1_1Rewriter.html) to inject `__inspector_*` function calls at strategic points
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

The Clang [`Rewriter`](https://clang.llvm.org/doxygen/classclang_1_1Rewriter.html) API works on **source text**, not the AST used by codegen. Therefore:
- A single `clang -fsyntax-only` pass produces instrumented source but won't compile it
- The instrumented source must be compiled in a separate pass
- This two-pass model is standard for source-rewriting tools ([clang-tidy](https://clang.llvm.org/extra/clang-tidy/), linters, etc.)

**Future optimization:** A single-pass compiler-integration via [LibTooling](https://clang.llvm.org/docs/LibTooling.html) or custom [LLVM IR](https://llvm.org/docs/LangRef.html) passes could eliminate the intermediate file, but would require significantly more code.

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
- **STL containers** (vector, string, array, pair, unique_ptr, shared_ptr, optional, variant, function)
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

The output follows the [OPT trace format](https://github.com/pgbovine/OnlinePythonTutor/blob/master/v3/docs/opt-trace-format.md) (used by [Python Tutor](https://pythontutor.com/) and similar visualization tools):

```json
{
  "code": "int main() { ... }",
  "trace": [
    {
      "event": "call",
      "func_name": "main",
      "line": 3,
      "stack_to_render": [...],
      "heap": {},
      "globals": {}
    }
  ]
}
```

Event types: `call`, `return`, `step_line`, `exception`, `catch`

See [docs/trace-format.md](docs/trace-format.md) for the complete specification.

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

The project includes a [SvelteKit](https://kit.svelte.dev/)-based visualization frontend in `web/`:

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

## Worthy Alternatives

Other tools for understanding C++ program execution:

| Tool | Approach | Best For |
|------|----------|----------|
| [Python Tutor C++](https://pythontutor.com/cpp.html) | Valgrind-based tracing | Quick visualization of simple programs |
| [Compiler Explorer](https://godbolt.org/) | Assembly output comparison | Understanding compiler optimizations |
| [C++ Insights](https://cppinsights.io/) | Source transformation viewer | Seeing what the compiler generates (templates, lambdas) |
| [RR](https://rr-project.org/) | Record & replay debugging | Deterministic debugging of complex bugs |
| [Valgrind](https://valgrind.org/) | Dynamic analysis | Memory leak and error detection |
| [AddressSanitizer](https://clang.llvm.org/docs/AddressSanitizer.html) | Compile-time instrumentation | Fast memory error detection |
| [GDB](https://www.gnu.org/software/gdb/) / [LLDB](https://lldb.llvm.org/) | Interactive debugging | Traditional step-through debugging |

**How we differ:** Most tools either interpret binaries (Valgrind) or require interactive debugging sessions (GDB). C++ Runtime Inspector uses Clang AST plugins to rewrite source code *before* compilation, injecting tracing hooks directly. This preserves variable names, type information, and produces a complete JSON trace—ideal for web-based visualizations and automated analysis.

## License

This project is a research artifact and educational tool. MIT License. See [LICENSE](LICENSE) for details.

## References

- [Clang Plugins Documentation](https://clang.llvm.org/docs/Plugins.html) — How to write Clang plugins
- [Introduction to the Clang AST](https://clang.llvm.org/docs/IntroductionToTheClangAST.html) — Understanding the AST structure
- [OPT Trace Format](https://github.com/pgbovine/OnlinePythonTutor/blob/master/v3/docs/opt-trace-format.md) — Python Tutor's trace specification
- [nlohmann/json](https://github.com/nlohmann/json) — JSON library used by the runtime
