//! @file inspector/Dynamic.h
//! @brief Runtime resolution of dynamic (most-derived) type names for
//!        polymorphic objects. Used to annotate struct encodings so that a
//!        `Shape*` pointing at a `Circle` is reported as a `Circle` in the
//!        trace.

#pragma once

#include <string>

namespace inspector {

//! Read the vtable pointer at `obj` and resolve it to a demangled class name.
//!
//! Uses `dladdr` to map the vtable pointer to its exported symbol, which on
//! Itanium-ABI compilers (g++, clang on Linux/macOS) follows the form
//! `_ZTVN<class-mangling>E`. The leading `_ZTV` is stripped and the rest is
//! demangled via `abi::__cxa_demangle`.
//!
//! Returns an empty string when:
//!   - `obj` is null
//!   - the vtable pointer can't be resolved (e.g. binary not built with
//!     `-rdynamic`, dynamic loader unavailable)
//!   - the symbol isn't a vtable
std::string resolveDynamicTypeName(const void* obj);

} // namespace inspector
