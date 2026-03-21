# Top-level build for the C++ Runtime Inspector.
#
# Layout:
#   plugin/    The Clang AST plugin (a shared library Clang loads).
#   runtime/   The tracing runtime, linked into instrumented binaries.
#   test/      A sample user program to instrument.
#   tests/     Golden tests for validation.
#
# Build prerequisites (Ubuntu 22.04+ / Kubuntu 24.04):
#   sudo apt install llvm-dev libclang-dev clang cmake build-essential
#
# The plugin must be built against the SAME LLVM/Clang version that you'll
# use to compile the user program. Mismatch causes hard-to-diagnose crashes
# in Clang's plugin loader.
#
# Usage after building:
#   1. Build:               cmake -B build && cmake --build build
#   2. Inspect rewrite:     clang++ -fplugin=build/plugin/libInspectorPlugin.so \
#                                   -fsyntax-only test/example.cpp
#   3. Compile + run:       ./scripts/instrument-and-run.sh

cmake_minimum_required(VERSION 3.20)
project(cpp_runtime_inspector C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Dependencies via FetchContent ----------------------------------------
include(FetchContent)

FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(json)

# --- LLVM/Clang discovery -------------------------------------------------
# find_package picks up the LLVMConfig.cmake / ClangConfig.cmake installed by
# the distro's llvm-dev / libclang-dev packages.
find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
message(STATUS "Using ClangConfig.cmake in: ${Clang_DIR}")

# Required for LLVM headers and macros to compile correctly.
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# --- The plugin ----------------------------------------------------------
# Plugins are SHARED libraries that Clang dlopens at compile time. They link
# against Clang's internal libraries — those are provided by Clang itself at
# load time, so we don't link them here (doing so causes "duplicate symbol"
# errors at runtime). This is the Clang-plugin convention.
add_library(InspectorPlugin SHARED
    plugin/InspectorPlugin.cpp
    plugin/Visitor.cpp
    plugin/TypeEncoder.cpp
    plugin/RewriteHelpers.cpp
    plugin/Diagnostics.cpp
)
target_include_directories(InspectorPlugin PRIVATE plugin)
target_compile_options(InspectorPlugin PRIVATE -fno-rtti)  # match Clang's build flags

# On macOS, Clang plugins must allow undefined symbols that are resolved at load time.
if(APPLE)
    target_link_options(InspectorPlugin PRIVATE -undefined dynamic_lookup)
endif()

# --- The runtime ---------------------------------------------------------
# Static archive that gets linked into instrumented programs. Could equally
# be shared; static is simpler for a PoC because there's no LD_LIBRARY_PATH
# to manage.
add_library(inspector_runtime STATIC
    runtime/inspector_runtime.cpp
    runtime/inspector/TypeInfo.cpp
    runtime/inspector/Trace.cpp
    runtime/inspector/JsonEmit.cpp
    runtime/inspector/Hooks.cpp
    runtime/inspector/StringSafe.cpp
    runtime/inspector/Heap.cpp
)
target_include_directories(inspector_runtime PUBLIC runtime)
target_link_libraries(inspector_runtime PRIVATE nlohmann_json::nlohmann_json)

# --- Testing -------------------------------------------------------------
enable_testing()

# Custom target to run golden tests
add_custom_target(golden-tests
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/run-golden-tests.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS InspectorPlugin inspector_runtime
    COMMENT "Running golden tests"
)
