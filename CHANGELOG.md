# Changelog

All notable changes to the See++ project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

### Added
- **Tier 3 Implementation** - Heap and pointer semantics
  - Heap allocation tracking with interval tree (sorted vector implementation)
  - `CXXNewExpr` instrumentation for `new` and `new[]` expressions
  - `CXXDeleteExpr` instrumentation for `delete` and `delete[]` expressions
  - C++ template wrappers: `__see_capture_new<T>`, `__see_capture_new_array<T>`
  - Pre-delete hooks: `__see_pre_delete<T>`, `__see_pre_delete_array<T>`
  - Pointer-to-heap resolution with `["REF", heap_id]` encoding
  - Offset resolution with `["REF_OFFSET", heap_id, offset]` encoding
  - Use-after-free detection with `["DANGLING", heap_id]` encoding
  - Memory leak detection at program exit (`memory_leaks` array in output)
  - Heap state snapshots in each trace step
  - Heap objects encoded as `HEAP_PRIMITIVE`, `HEAP_ARRAY`, or `HEAP_STRUCT`
  - Golden tests for heap tracking (heap_basic, heap_array, heap_uaf, heap_leak)

- **Tier 2 Implementation** - User-defined types support
  - Struct/class field enumeration and encoding
  - Single inheritance support with base class field inclusion
  - Polymorphic class support (is_polymorphic flag for virtual functions)
  - Scoped and unscoped enum support with symbolic name display
  - Union support with raw byte representation and first field interpretation
  - Fixed-size array support with element-by-element encoding
  - Type descriptor generation for all composite types
  - Composite type hooks: `__see_var_init_struct`, `__see_var_init_enum`,
    `__see_var_init_union`, `__see_var_init_array` (with update variants)
  - Golden tests for structs, enums

- **Tier 1 Implementation** - Complete rewrite of the See++ instrumentation framework
  - Modular plugin architecture with separated concerns:
    - `Visitor.cpp` - AST traversal and instrumentation
    - `TypeEncoder.cpp` - Type descriptor generation
    - `RewriteHelpers.cpp` - Source rewriting utilities
    - `Diagnostics.cpp` - Warning and error emission
  - Modular runtime architecture:
    - `see/Trace.cpp` - Trace state management
    - `see/TypeInfo.cpp` - Type descriptors
    - `see/JsonEmit.cpp` - OPT format JSON emission
    - `see/Hooks.cpp` - Instrumentation hook implementations
    - `see/StringSafe.cpp` - SIGSEGV-protected string reading
  - Type-specific variable tracking hooks:
    - `__see_var_init_int`, `__see_var_update_int`
    - `__see_var_init_uint`, `__see_var_update_uint`
    - `__see_var_init_float`, `__see_var_update_float`
    - `__see_var_init_bool`, `__see_var_update_bool`
    - `__see_var_init_char`, `__see_var_update_char`
    - `__see_var_init_ptr`, `__see_var_update_ptr`
    - `__see_var_init_ref`, `__see_var_update_ref`
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

## [0.1.0] - Initial PoC

### Added
- Basic Clang plugin for source instrumentation
- Runtime library for trace collection
- Support for `int` variable tracking
- Function entry/exit tracking
- Simple JSON trace output
- Three-pass build script (instrument, compile, link)
