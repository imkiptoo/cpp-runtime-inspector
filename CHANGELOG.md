# Changelog

All notable changes to the See++ project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

### Added
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
