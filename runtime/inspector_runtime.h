//! @file inspector_runtime.h
//! @brief Public interface of the C++ Runtime Inspector tracing runtime.
//!
//! These functions are called by code instrumented by InspectorPlugin.
//! They accumulate trace events and emit a JSON trace at exit.
//!
//! The type-specific hooks pass values in their natural representation
//! to avoid information loss from casting to int.

#pragma once

#include "inspector/TypeInfo.h"

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------
// Function entry/exit
// ---------------------------------------------------------------------------

//! Called when execution enters a function.
void __inspector_enter(const char* funcName, int line);

//! Called when execution leaves a function.
void __inspector_leave(const char* funcName, int line);

// ---------------------------------------------------------------------------
// Line stepping
// ---------------------------------------------------------------------------

//! Called before each statement to record line position.
void __inspector_step(int line);

// ---------------------------------------------------------------------------
// Type-specific variable hooks
// ---------------------------------------------------------------------------

// Integer types (signed)
void __inspector_var_init_int(const char* name, void* addr,
                         const inspector::TypeDescriptor* type, long long value,
                         int line);
void __inspector_var_update_int(const char* name, void* addr,
                           const inspector::TypeDescriptor* type, long long value);

// Integer types (unsigned)
void __inspector_var_init_uint(const char* name, void* addr,
                          const inspector::TypeDescriptor* type,
                          unsigned long long value, int line);
void __inspector_var_update_uint(const char* name, void* addr,
                            const inspector::TypeDescriptor* type,
                            unsigned long long value);

// Floating point types
void __inspector_var_init_float(const char* name, void* addr,
                           const inspector::TypeDescriptor* type, double value,
                           int line);
void __inspector_var_update_float(const char* name, void* addr,
                             const inspector::TypeDescriptor* type, double value);

// Boolean type
void __inspector_var_init_bool(const char* name, void* addr,
                          const inspector::TypeDescriptor* type, bool value, int line);
void __inspector_var_update_bool(const char* name, void* addr,
                            const inspector::TypeDescriptor* type, bool value);

// Character type
void __inspector_var_init_char(const char* name, void* addr,
                          const inspector::TypeDescriptor* type, int value, int line);
void __inspector_var_update_char(const char* name, void* addr,
                            const inspector::TypeDescriptor* type, int value);

// Pointer types
void __inspector_var_init_ptr(const char* name, void* addr,
                         const inspector::TypeDescriptor* type, const void* ptr_value,
                         int line);
void __inspector_var_update_ptr(const char* name, void* addr,
                           const inspector::TypeDescriptor* type,
                           const void* ptr_value);

// Reference types
void __inspector_var_init_ref(const char* name, void* addr,
                         const inspector::TypeDescriptor* type,
                         const void* referent_addr, int line);
void __inspector_var_update_ref(const char* name, void* addr,
                           const inspector::TypeDescriptor* type,
                           const void* referent_addr);

// ---------------------------------------------------------------------------
// Tier 2: Composite types
// ---------------------------------------------------------------------------

// Struct/class types (value is the struct address for field traversal)
void __inspector_var_init_struct(const char* name, void* addr,
                            const inspector::TypeDescriptor* type, int line);
void __inspector_var_update_struct(const char* name, void* addr,
                              const inspector::TypeDescriptor* type);

// Enum types
void __inspector_var_init_enum(const char* name, void* addr,
                          const inspector::TypeDescriptor* type, long long value,
                          int line);
void __inspector_var_update_enum(const char* name, void* addr,
                            const inspector::TypeDescriptor* type, long long value);

// Union types
void __inspector_var_init_union(const char* name, void* addr,
                           const inspector::TypeDescriptor* type, int line);
void __inspector_var_update_union(const char* name, void* addr,
                             const inspector::TypeDescriptor* type);

// Array types (fixed-size)
void __inspector_var_init_array(const char* name, void* addr,
                           const inspector::TypeDescriptor* type, int line);
void __inspector_var_update_array(const char* name, void* addr,
                             const inspector::TypeDescriptor* type);

// ---------------------------------------------------------------------------
// Tier 3: Heap allocation tracking
// ---------------------------------------------------------------------------

//! Record a heap allocation. Returns heap ID.
int __inspector_alloc(void* ptr, unsigned long size, const inspector::TypeDescriptor* type,
                 int is_array, unsigned long array_count);

//! Record a heap deallocation (before the actual free/delete).
void __inspector_dealloc(void* ptr);

// ---------------------------------------------------------------------------
// Legacy compatibility (Tier 0)
// ---------------------------------------------------------------------------

//! @deprecated Use __inspector_var_init_int instead.
void __inspector_var_init(const char* name, void* addr, int value);

//! @deprecated Use __inspector_var_update_int instead.
void __inspector_var_update(const char* name, void* addr, int value);

#ifdef __cplusplus
}

// ---------------------------------------------------------------------------
// C++ template wrappers for new/delete capture
// ---------------------------------------------------------------------------

namespace inspector {

//! Capture a single-object new expression.
//! Usage: T* p = inspector::__inspector_capture_new<T>(new T(args), &__inspector_type_T);
template <typename T>
T* __inspector_capture_new(T* ptr, const TypeDescriptor* type) {
    if (ptr) {
        __inspector_alloc(ptr, sizeof(T), type, 0, 1);
    }
    return ptr;
}

//! Capture an array new expression.
//! Usage: T* p = inspector::__inspector_capture_new_array<T>(new T[n], &__inspector_type_T, n);
template <typename T>
T* __inspector_capture_new_array(T* ptr, const TypeDescriptor* type, unsigned long count) {
    if (ptr) {
        __inspector_alloc(ptr, sizeof(T) * count, type, 1, count);
    }
    return ptr;
}

//! Pre-delete hook for single object.
//! Usage: (inspector::__inspector_pre_delete(p), delete p);
template <typename T>
void __inspector_pre_delete(T* ptr) {
    if (ptr) {
        __inspector_dealloc(ptr);
    }
}

//! Pre-delete hook for array.
//! Usage: (inspector::__inspector_pre_delete_array(p), delete[] p);
template <typename T>
void __inspector_pre_delete_array(T* ptr) {
    if (ptr) {
        __inspector_dealloc(ptr);
    }
}

} // namespace inspector

#endif // __cplusplus

