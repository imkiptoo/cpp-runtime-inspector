//! @file see_runtime.h
//! @brief Public interface of the See++ tracing runtime.
//!
//! These functions are called by code instrumented by SeePlugin.
//! They accumulate trace events and emit a JSON trace at exit.
//!
//! The type-specific hooks pass values in their natural representation
//! to avoid information loss from casting to int.

#pragma once

#include "see/TypeInfo.h"

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------
// Function entry/exit
// ---------------------------------------------------------------------------

//! Called when execution enters a function.
void __see_enter(const char* funcName, int line);

//! Called when execution leaves a function.
void __see_leave(const char* funcName, int line);

// ---------------------------------------------------------------------------
// Line stepping
// ---------------------------------------------------------------------------

//! Called before each statement to record line position.
void __see_step(int line);

// ---------------------------------------------------------------------------
// Type-specific variable hooks
// ---------------------------------------------------------------------------

// Integer types (signed)
void __see_var_init_int(const char* name, void* addr,
                        const see::TypeDescriptor* type, long long value,
                        int line);
void __see_var_update_int(const char* name, void* addr,
                          const see::TypeDescriptor* type, long long value);

// Integer types (unsigned)
void __see_var_init_uint(const char* name, void* addr,
                         const see::TypeDescriptor* type,
                         unsigned long long value, int line);
void __see_var_update_uint(const char* name, void* addr,
                           const see::TypeDescriptor* type,
                           unsigned long long value);

// Floating point types
void __see_var_init_float(const char* name, void* addr,
                          const see::TypeDescriptor* type, double value,
                          int line);
void __see_var_update_float(const char* name, void* addr,
                            const see::TypeDescriptor* type, double value);

// Boolean type
void __see_var_init_bool(const char* name, void* addr,
                         const see::TypeDescriptor* type, bool value, int line);
void __see_var_update_bool(const char* name, void* addr,
                           const see::TypeDescriptor* type, bool value);

// Character type
void __see_var_init_char(const char* name, void* addr,
                         const see::TypeDescriptor* type, int value, int line);
void __see_var_update_char(const char* name, void* addr,
                           const see::TypeDescriptor* type, int value);

// Pointer types
void __see_var_init_ptr(const char* name, void* addr,
                        const see::TypeDescriptor* type, const void* ptr_value,
                        int line);
void __see_var_update_ptr(const char* name, void* addr,
                          const see::TypeDescriptor* type,
                          const void* ptr_value);

// Reference types
void __see_var_init_ref(const char* name, void* addr,
                        const see::TypeDescriptor* type,
                        const void* referent_addr, int line);
void __see_var_update_ref(const char* name, void* addr,
                          const see::TypeDescriptor* type,
                          const void* referent_addr);

// ---------------------------------------------------------------------------
// Tier 2: Composite types
// ---------------------------------------------------------------------------

// Struct/class types (value is the struct address for field traversal)
void __see_var_init_struct(const char* name, void* addr,
                           const see::TypeDescriptor* type, int line);
void __see_var_update_struct(const char* name, void* addr,
                             const see::TypeDescriptor* type);

// Enum types
void __see_var_init_enum(const char* name, void* addr,
                         const see::TypeDescriptor* type, long long value,
                         int line);
void __see_var_update_enum(const char* name, void* addr,
                           const see::TypeDescriptor* type, long long value);

// Union types
void __see_var_init_union(const char* name, void* addr,
                          const see::TypeDescriptor* type, int line);
void __see_var_update_union(const char* name, void* addr,
                            const see::TypeDescriptor* type);

// Array types (fixed-size)
void __see_var_init_array(const char* name, void* addr,
                          const see::TypeDescriptor* type, int line);
void __see_var_update_array(const char* name, void* addr,
                            const see::TypeDescriptor* type);

// ---------------------------------------------------------------------------
// Tier 3: Heap allocation tracking
// ---------------------------------------------------------------------------

//! Record a heap allocation. Returns heap ID.
int __see_alloc(void* ptr, unsigned long size, const see::TypeDescriptor* type,
                int is_array, unsigned long array_count);

//! Record a heap deallocation (before the actual free/delete).
void __see_dealloc(void* ptr);

// ---------------------------------------------------------------------------
// Legacy compatibility (Tier 0)
// ---------------------------------------------------------------------------

//! @deprecated Use __see_var_init_int instead.
void __see_var_init(const char* name, void* addr, int value);

//! @deprecated Use __see_var_update_int instead.
void __see_var_update(const char* name, void* addr, int value);

#ifdef __cplusplus
}

// ---------------------------------------------------------------------------
// C++ template wrappers for new/delete capture
// ---------------------------------------------------------------------------

namespace see {

//! Capture a single-object new expression.
//! Usage: T* p = see::__see_capture_new<T>(new T(args), &__see_type_T);
template <typename T>
T* __see_capture_new(T* ptr, const TypeDescriptor* type) {
    if (ptr) {
        __see_alloc(ptr, sizeof(T), type, 0, 1);
    }
    return ptr;
}

//! Capture an array new expression.
//! Usage: T* p = see::__see_capture_new_array<T>(new T[n], &__see_type_T, n);
template <typename T>
T* __see_capture_new_array(T* ptr, const TypeDescriptor* type, unsigned long count) {
    if (ptr) {
        __see_alloc(ptr, sizeof(T) * count, type, 1, count);
    }
    return ptr;
}

//! Pre-delete hook for single object.
//! Usage: (see::__see_pre_delete(p), delete p);
template <typename T>
void __see_pre_delete(T* ptr) {
    if (ptr) {
        __see_dealloc(ptr);
    }
}

//! Pre-delete hook for array.
//! Usage: (see::__see_pre_delete_array(p), delete[] p);
template <typename T>
void __see_pre_delete_array(T* ptr) {
    if (ptr) {
        __see_dealloc(ptr);
    }
}

} // namespace see

#endif // __cplusplus
