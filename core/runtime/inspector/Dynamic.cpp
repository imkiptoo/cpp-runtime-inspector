//! @file inspector/Dynamic.cpp
//! @brief Vtable→class-name resolution via dladdr + Itanium demangling.

#include "Dynamic.h"

#include <cstdlib>
#include <cstring>
#include <string>

#if defined(__has_include)
#  if __has_include(<dlfcn.h>)
#    include <dlfcn.h>
#    define INSPECTOR_HAS_DLADDR 1
#  endif
#  if __has_include(<cxxabi.h>)
#    include <cxxabi.h>
#    define INSPECTOR_HAS_CXA_DEMANGLE 1
#  endif
#endif

namespace inspector {

std::string resolveDynamicTypeName(const void* obj) {
#if defined(INSPECTOR_HAS_DLADDR) && defined(INSPECTOR_HAS_CXA_DEMANGLE)
    if (!obj) {
        return {};
    }

    // The vtable pointer occupies the first slot of any polymorphic object
    // under the Itanium C++ ABI. Read it as a void*.
    const void* vptr = nullptr;
    std::memcpy(&vptr, obj, sizeof(const void*));
    if (!vptr) {
        return {};
    }

    // Resolve the vtable address to its exported symbol via dladdr.
    Dl_info info{};
    if (!dladdr(vptr, &info) || !info.dli_sname) {
        return {};
    }

    // Vtable symbols are mangled as `_ZTV<MangledClass>` in the Itanium ABI.
    // Strip the leading `_ZTV` and feed the rest to __cxa_demangle as a
    // type spelling. We synthesize `_ZN<rest>` so that the demangler
    // produces the bare class name.
    const char* sym = info.dli_sname;
    static constexpr const char kVtablePrefix[] = "_ZTV";
    static constexpr size_t kVtablePrefixLen = sizeof(kVtablePrefix) - 1;
    if (std::strncmp(sym, kVtablePrefix, kVtablePrefixLen) != 0) {
        return {};
    }

    // __cxa_demangle expects an entire mangled name. Rebuild one as if it
    // were a function symbol: `_Z<rest>v` produces "ClassName(void)" — we
    // want the type name only. Easier path: prepend `_Z` (no T-V) and let
    // demangling produce something like "ClassName::~ClassName()" — that
    // also includes extra. Cleanest: use the type-name demangler form by
    // wrapping the rest as a `<unscoped-name>` sequence inside a fake
    // mangled prefix `_ZN...E`. But __cxa_demangle handles the literal
    // `_ZTV...` form too — it returns "vtable for ClassName" — which we
    // can post-process by stripping the leading "vtable for ".
    int status = 0;
    char* demangled =
        abi::__cxa_demangle(sym, /*output_buffer*/ nullptr,
                            /*length*/ nullptr, &status);
    if (status != 0 || !demangled) {
        if (demangled) std::free(demangled);
        return {};
    }

    std::string result(demangled);
    std::free(demangled);

    // The demangler returns "vtable for ClassName". Strip the prefix.
    static constexpr const char kVtableForPrefix[] = "vtable for ";
    static constexpr size_t kVtableForLen = sizeof(kVtableForPrefix) - 1;
    if (result.compare(0, kVtableForLen, kVtableForPrefix) == 0) {
        result.erase(0, kVtableForLen);
    }
    return result;
#else
    (void)obj;
    return {};
#endif
}

} // namespace inspector
