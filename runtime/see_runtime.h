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
// Legacy compatibility (Tier 0)
// ---------------------------------------------------------------------------

//! @deprecated Use __see_var_init_int instead.
void __see_var_init(const char* name, void* addr, int value);

//! @deprecated Use __see_var_update_int instead.
void __see_var_update(const char* name, void* addr, int value);

#ifdef __cplusplus
}
#endif
