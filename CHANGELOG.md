# Changelog

All notable changes to the C++ Runtime Inspector project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

### Added
- **Tier 6 Implementation - COMPLETE** - Production polish
  - Crash signal handlers (`runtime/inspector/Signals.cpp`)
    - SIGSEGV, SIGABRT, SIGFPE, SIGBUS, SIGILL handling
    - Async-signal-safe crash message output
    - Trace emission before process termination
  - Resource limits for protection
    - Event count limit: default 100,000 events
    - Output size limit: default 50MB with truncation
    - `TraceState::setMaxEvents()` and `JsonEmitter::emit()` size parameter
  - Sandboxing infrastructure (`sandbox/`)
    - `Dockerfile` - Multi-stage build for minimal runtime image
    - `run-traced.sh` - Entry point script with resource limits
    - `seccomp-profile.json` - Syscall whitelist for security
    - `docker-compose.yml` - Security-hardened container config
  - Frontend adapter HTTP service (`frontend-adapter/`)
    - `server.py` - Python HTTP server accepting C++ source
    - POST /trace endpoint returns OPT-format JSON
    - GET /health for service health checks
    - CORS support for browser-based frontends
  - CI/CD pipeline (`.github/workflows/ci.yml`)
    - Build matrix: Clang 17/18/19 × Ubuntu 22.04/24.04
    - Golden test suite execution
    - Docker sandbox build and test
    - Documentation check
  - Documentation (`docs/`)
    - `trace-format.md` - Complete OPT format specification
    - `architecture.md` - System design and internals
    - `supported-language-subset.md` - Supported C++ features
    - `extending-stl-encoders.md` - How to add new containers
    - `frontend-integration.md` - Frontend consumption guide
  - Additional golden tests
    - `deep_recursion` - Stack frame handling
    - `large_output` - Event handling
    - `many_variables` - Variable tracking
    - `mixed_types` - Type combination handling
  - Crash handler test script (`scripts/test-crash-handler.sh`)

- **Tier 5 Implementation** - Control flow special cases
  - Compound assignment operator tracking (`+=`, `-=`, `*=`, `/=`, etc.)
    - `VisitCompoundAssignOperator()` in Visitor.cpp
  - Pre/post increment/decrement tracking (`++x`, `x++`, `--x`, `x--`)
    - `VisitUnaryOperator()` for increment/decrement ops
    - Proper handling when used in variable initializers
  - Exception tracking
    - `VisitCXXThrowExpr()` for throw expression instrumentation
    - `VisitCXXCatchStmt()` for catch block entry tracking
    - `__inspector_throw()` and `__inspector_catch()` runtime hooks
    - New event types: `"exception"` and `"catch"`
  - Fixed exception variable instrumentation (skip catch clause parameters)
  - Fixed variable initializer conflicts with operator instrumentation
  - Golden tests for compound_assign, increment, exceptions, return_value, globals

- **Tier 4 Implementation** - Templates and STL support
  - STL container encoders (`runtime/inspector/StlEncoders.cpp`)
    - `std::vector<T>` - encodes elements as array
    - `std::string` - SSO-aware string content extraction
    - `std::array<T, N>` - fixed-size array encoding
    - `std::pair<T1, T2>` - tuple-like encoding
    - `std::unique_ptr<T>` - smart pointer with heap resolution
    - `std::shared_ptr<T>` - smart pointer encoding
    - `std::optional<T>` - optional value encoding (placeholder)
    - `std::map<K, V>` / `std::set<T>` - placeholder for tree traversal
  - STL container detection via regex pattern matching on type names
  - Lambda support with friendly naming (`<lambda#1>`, etc.)
  - Template instantiation type descriptor generation
  - `TypeEncoder::isStlContainer()`, `isLambda()`, `getLambdaFriendlyName()`
  - `TypeEncoder::getStlElementType()` for extracting container element types
  - `TraceState::encodeValueAtAddress()` for STL-aware encoding
  - Golden tests for STL (stl_vector, stl_string, stl_unique_ptr)
  - Golden tests for lambdas and template functions
  - Additional edge case tests (pointer_arithmetic, nested_structs, inheritance, multi_function)

- **Tier 3 Implementation** - Heap and pointer semantics
  - Heap allocation tracking with interval tree (sorted vector implementation)
  - `CXXNewExpr` instrumentation for `new` and `new[]` expressions
  - `CXXDeleteExpr` instrumentation for `delete` and `delete[]` expressions
  - C++ template wrappers: `__inspector_capture_new<T>`, `__inspector_capture_new_array<T>`
  - Pre-delete hooks: `__inspector_pre_delete<T>`, `__inspector_pre_delete_array<T>`
  - Pointer-to-heap resolution with `["REF", heap_id]` encoding
  - Offset resolution with `["REF_OFFSET", heap_id, offset]` encoding
  - Use-after-free detection with `["DANGLING", heap_id]` encoding
  - Memory leak detection at program exit (`memory_leaks` array in output)
  - Heap state snapshots in each trace step
  - Heap objects encoded as `HEAP_PRIMITIVE`, `HEAP_ARRAY`, or `HEAP_STRUCT`
  - **malloc/free shim** (`shim/inspector_malloc_shim.c`) for C-style allocation tracking
    - Intercepts malloc, calloc, realloc, free, posix_memalign
    - Uses dlsym(RTLD_NEXT, ...) for portability across Linux and macOS
    - Thread-local reentrancy guard prevents infinite recursion
    - Bootstrap buffer for allocations during dlsym initialization
    - Runtime hooks: `__inspector_alloc_malloc`, `__inspector_dealloc_malloc`
  - Golden tests for heap tracking (heap_basic, heap_array, heap_uaf, heap_leak, linked_list, malloc_basic)
  - Test runner support for shim-based tests (`.use_shim` marker file)

- **Tier 2 Implementation** - User-defined types support
  - Struct/class field enumeration and encoding
  - Single inheritance support with base class field inclusion
  - Polymorphic class support (is_polymorphic flag for virtual functions)
  - Scoped and unscoped enum support with symbolic name display
  - Union support with raw byte representation and first field interpretation
  - Fixed-size array support with element-by-element encoding
  - Type descriptor generation for all composite types
  - Composite type hooks: `__inspector_var_init_struct`, `__inspector_var_init_enum`,
    `__inspector_var_init_union`, `__inspector_var_init_array` (with update variants)
  - Golden tests for structs, enums

- **Tier 1 Implementation** - Complete rewrite of the C++ Runtime Inspector instrumentation framework
  - Modular plugin architecture with separated concerns:
    - `Visitor.cpp` - AST traversal and instrumentation
    - `TypeEncoder.cpp` - Type descriptor generation
    - `RewriteHelpers.cpp` - Source rewriting utilities
    - `Diagnostics.cpp` - Warning and error emission
  - Modular runtime architecture:
    - `inspector/Trace.cpp` - Trace state management
    - `inspector/TypeInfo.cpp` - Type descriptors
    - `inspector/JsonEmit.cpp` - OPT format JSON emission
    - `inspector/Hooks.cpp` - Instrumentation hook implementations
    - `inspector/StringSafe.cpp` - SIGSEGV-protected string reading
  - Type-specific variable tracking hooks:
    - `__inspector_var_init_int`, `__inspector_var_update_int`
    - `__inspector_var_init_uint`, `__inspector_var_update_uint`
    - `__inspector_var_init_float`, `__inspector_var_update_float`
    - `__inspector_var_init_bool`, `__inspector_var_update_bool`
    - `__inspector_var_init_char`, `__inspector_var_update_char`
    - `__inspector_var_init_ptr`, `__inspector_var_update_ptr`
    - `__inspector_var_init_ref`, `__inspector_var_update_ref`
  - Full OPT trace format with:
    - Stack frame snapshots
    - Variable state tracking
    - Function entry/exit events
    - Line number tracking for function events
  - Golden test infrastructure with JSON-aware comparison
  - nlohmann/json library integration via FetchContent
  - Line-aware step injection (only for CompoundStmt children)
  - Automatic brace insertion for single-statement control flow bodies
  - Pointer and reference tracking with memory region classification
  - Safe string reading with SIGSEGV protection

### Changed
- Function entry/exit hooks now include line numbers
- Trace output uses proper OPT format instead of simplified format
- Plugin uses modular architecture instead of single monolithic file

### Fixed
- Static singleton destruction order issue with atexit handler
- Variable init calls now properly placed after semicolons

## [1.0.2] - 2026-06-21

### Added
- `justfile` task runner with `just <verb> <noun>` recipes (build/run/test/check).
- `BUILD_DIR` support in `scripts/update-expected.sh` for regenerating
  `*.linux.json` goldens from a Linux build.

### Changed
- Instrumentation now emits a trace step for **every executed line** —
  expression statements (`std::cout`, function calls) and reassignments are
  stepped, not just declarations.
- Lowered the runtime event cap from 100,000 to 5,000 so loops (which now step
  per iteration) cannot blow up the trace JSON.

### Fixed
- Light mode now follows the in-app theme instead of the OS (`@custom-variant
  dark` for Tailwind v4); polished light-mode panels and canvas.
- Output/Build console pane renders correctly in light mode.
- `int` declarations and parameters now record the correct source line (the
  legacy var-init hook hard-coded line 0).
- Web dev proxy targets the configured backend port (`BACKEND_PORT`, default
  8090) instead of the frequently-occupied 8080.

## [0.1.0] - Initial PoC

### Added
- Basic Clang plugin for source instrumentation
- Runtime library for trace collection
- Support for `int` variable tracking
- Function entry/exit tracking
- Simple JSON trace output
- Three-pass build script (instrument, compile, link)
