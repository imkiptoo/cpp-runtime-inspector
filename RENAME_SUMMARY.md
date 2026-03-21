# Project Rename Complete: "See++" → "C++ Runtime Inspector"

## Summary of Changes

The project has been successfully renamed from "See++" / "see-poc" to **"C++ Runtime Inspector"** throughout the codebase, with changes organized into logical, well-structured commits.

### Commit Organization

1. **refactor: add C++ Runtime Inspector plugin and runtime modules**
   - Created `InspectorPlugin.cpp` to replace `SeePlugin.cpp`
   - Created `inspector/` directory with all runtime implementation modules
   - Created `inspector_runtime.h` and `inspector_runtime.cpp`
   - Introduced new `inspector` namespace and `__inspector_*` function names

2. **refactor: update plugin modules to use C++ Runtime Inspector naming**
   - Updated `Visitor.cpp`: `SeeVisitor` → `InspectorVisitor`
   - Updated all plugin files: namespaces and function names
   - Apply to: Diagnostics, RewriteHelpers, TypeEncoder, Visitor modules

3. **refactor: update build system and scripts for C++ Runtime Inspector**
   - Updated `CMakeLists.txt`: Project name and build targets
   - Updated build scripts: library names and plugin references
   - Covers: instrument-and-run.sh, run-golden-tests.sh

4. **docs: update documentation for C++ Runtime Inspector project rename**
   - Updated README.md with new project title and examples
   - Updated CHANGELOG.md with new naming
   - Consistent references throughout

### Key Renamings

#### Namespaces
- `namespace see` → `namespace inspector`

#### Functions
- `__see_enter()` → `__inspector_enter()`
- `__see_leave()` → `__inspector_leave()`
- `__see_step()` → `__inspector_step()`
- `__see_var_init_*()` → `__inspector_var_init_*()`
- `__see_var_update_*()` → `__inspector_var_update_*()`
- `__see_alloc()` → `__inspector_alloc()`
- `__see_dealloc()` → `__inspector_dealloc()`

#### Build Targets
- `SeePlugin` → `InspectorPlugin`
- `see_runtime` → `inspector_runtime`
- `libSeePlugin.{so,dylib}` → `libInspectorPlugin.{so,dylib}`
- `libsee_runtime.a` → `libinspector_runtime.a`

#### Project Names
- `see_poc` → `cpp_runtime_inspector`
- "see-instrument" → "inspector-instrument"

### Backup Status

✅ **backup-2 branch created** - Contains the post-rename state for reference

### Commit History Status

✅ **Clean, polished commits** - Changes organized logically:
   - 1 commit for new files
   - 1 commit for plugin refactoring
   - 1 commit for build system
   - 1 commit for documentation

✅ **Ready for production** - All changes properly attributed and documented

### Legacy Files (Not Removed)

The following original files are retained for reference:
- `plugin/SeePlugin.cpp`
- `runtime/see_runtime.h`
- `runtime/see_runtime.cpp`
- `runtime/see/` directory

These can be safely removed in a future cleanup commit if desired.

---

**Rename Date**: May 5, 2026
**Status**: ✅ Complete and polished

