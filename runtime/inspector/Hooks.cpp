//! @file inspector/Hooks.cpp
//! @brief Implementation of __inspector_* hook functions.

#include "JsonEmit.h"
#include "Trace.h"
#include "TypeInfo.h"
#include "StringSafe.h"

#include <atomic>
#include <cstdlib>
#include <iomanip>
#include <sstream>

namespace inspector {

namespace {

//! Format a pointer address as hex string.
std::string formatAddress(const void* ptr) {
    std::ostringstream ss;
    ss << "0x" << std::hex << reinterpret_cast<uintptr_t>(ptr);
    return ss.str();
}

//! Encode a pointer value using TraceState's heap-aware encoding.
EncodedValue encodePointerValue(const void* ptr, const TypeDescriptor* type) {
    return TraceState::instance().encodePointer(ptr, type);
}

//! Idempotent atexit registration.
void ensureExitHandlerRegistered() {
    static std::atomic<bool> registered{false};
    bool expected = false;
    if (registered.compare_exchange_strong(expected, true)) {
        std::atexit([]() {
            auto& state = TraceState::instance();
            if (!state.isFinalized()) {
                // Check for memory leaks and emit events
                state.checkLeaks();
                state.finalize();
                JsonEmitter::emit(state);
            }
        });
    }
}

} // anonymous namespace

} // namespace inspector

// ---------------------------------------------------------------------------
// Public C API
// ---------------------------------------------------------------------------

extern "C" {

void __inspector_enter(const char* funcName, int line) {
    inspector::ensureExitHandlerRegistered();
    inspector::TraceState::instance().pushFrame(funcName ? funcName : "<null>", line);
}

void __inspector_leave(const char* funcName, int line) {
    (void)funcName; // Used for validation in debug builds
    inspector::TraceState::instance().popFrame(line);
}

void __inspector_step(int line) {
    inspector::TraceState::instance().recordStep(line);
}

// --- Integer types ---

void __inspector_var_init_int(const char* name, void* addr,
                        const inspector::TypeDescriptor* type, long long value,
                        int line) {
    inspector::TraceState::instance().recordVarInit(
        name, addr, type, inspector::EncodedValue{value}, line);
}

void __inspector_var_update_int(const char* name, void* addr,
                          const inspector::TypeDescriptor* type, long long value) {
    inspector::TraceState::instance().recordVarUpdate(name, addr, type,
                                                 inspector::EncodedValue{value});
}

void __inspector_var_init_uint(const char* name, void* addr,
                         const inspector::TypeDescriptor* type,
                         unsigned long long value, int line) {
    inspector::TraceState::instance().recordVarInit(
        name, addr, type, inspector::EncodedValue{value}, line);
}

void __inspector_var_update_uint(const char* name, void* addr,
                           const inspector::TypeDescriptor* type,
                           unsigned long long value) {
    inspector::TraceState::instance().recordVarUpdate(name, addr, type,
                                                 inspector::EncodedValue{value});
}

// --- Floating point types ---

void __inspector_var_init_float(const char* name, void* addr,
                          const inspector::TypeDescriptor* type, double value,
                          int line) {
    inspector::TraceState::instance().recordVarInit(
        name, addr, type, inspector::EncodedValue{value}, line);
}

void __inspector_var_update_float(const char* name, void* addr,
                            const inspector::TypeDescriptor* type, double value) {
    inspector::TraceState::instance().recordVarUpdate(name, addr, type,
                                                 inspector::EncodedValue{value});
}

// --- Boolean type ---

void __inspector_var_init_bool(const char* name, void* addr,
                         const inspector::TypeDescriptor* type, bool value,
                         int line) {
    inspector::TraceState::instance().recordVarInit(
        name, addr, type, inspector::EncodedValue{value}, line);
}

void __inspector_var_update_bool(const char* name, void* addr,
                           const inspector::TypeDescriptor* type, bool value) {
    inspector::TraceState::instance().recordVarUpdate(name, addr, type,
                                                 inspector::EncodedValue{value});
}

// --- Character type ---

void __inspector_var_init_char(const char* name, void* addr,
                         const inspector::TypeDescriptor* type, int value,
                         int line) {
    inspector::TraceState::instance().recordVarInit(
        name, addr, type, inspector::EncodedValue{static_cast<char>(value)}, line);
}

void __inspector_var_update_char(const char* name, void* addr,
                           const inspector::TypeDescriptor* type, int value) {
    inspector::TraceState::instance().recordVarUpdate(
        name, addr, type, inspector::EncodedValue{static_cast<char>(value)});
}

// --- Pointer types ---

void __inspector_var_init_ptr(const char* name, void* addr,
                        const inspector::TypeDescriptor* type, const void* ptr_value,
                        int line) {
    inspector::EncodedValue value = inspector::encodePointerValue(ptr_value, type);

    // Special handling for C-strings
    if (type && type->element_type &&
        type->element_type->kind == inspector::TypeKind::Char) {
        std::string str = inspector::safeReadString(static_cast<const char*>(ptr_value), 256);
        if (!str.empty()) {
            value = str;
        }
    }

    inspector::TraceState::instance().recordVarInit(name, addr, type, std::move(value),
                                               line);
}

void __inspector_var_update_ptr(const char* name, void* addr,
                          const inspector::TypeDescriptor* type,
                          const void* ptr_value) {
    inspector::EncodedValue value = inspector::encodePointerValue(ptr_value, type);

    // Special handling for C-strings
    if (type && type->element_type &&
        type->element_type->kind == inspector::TypeKind::Char) {
        std::string str = inspector::safeReadString(static_cast<const char*>(ptr_value), 256);
        if (!str.empty()) {
            value = str;
        }
    }

    inspector::TraceState::instance().recordVarUpdate(name, addr, type,
                                                 std::move(value));
}

// --- Reference types ---

void __inspector_var_init_ref(const char* name, void* addr,
                        const inspector::TypeDescriptor* type,
                        const void* referent_addr, int line) {
    inspector::EncodedValue value = inspector::encodePointerValue(referent_addr, type);
    inspector::TraceState::instance().recordVarInit(name, addr, type, std::move(value),
                                               line);
}

void __inspector_var_update_ref(const char* name, void* addr,
                          const inspector::TypeDescriptor* type,
                          const void* referent_addr) {
    inspector::EncodedValue value = inspector::encodePointerValue(referent_addr, type);
    inspector::TraceState::instance().recordVarUpdate(name, addr, type,
                                                 std::move(value));
}

// ---------------------------------------------------------------------------
// Tier 2: Composite types
// ---------------------------------------------------------------------------

void __inspector_var_init_struct(const char* name, void* addr,
                           const inspector::TypeDescriptor* type, int line) {
    inspector::EncodedValue value = inspector::TraceState::instance().encodeStruct(addr, type);
    inspector::TraceState::instance().recordVarInit(name, addr, type, std::move(value),
                                               line);
}

void __inspector_var_update_struct(const char* name, void* addr,
                             const inspector::TypeDescriptor* type) {
    inspector::EncodedValue value = inspector::TraceState::instance().encodeStruct(addr, type);
    inspector::TraceState::instance().recordVarUpdate(name, addr, type, std::move(value));
}

void __inspector_var_init_enum(const char* name, void* addr,
                         const inspector::TypeDescriptor* type, long long value,
                         int line) {
    inspector::EncodedValue encoded = inspector::TraceState::instance().encodeEnum(value, type);
    inspector::TraceState::instance().recordVarInit(name, addr, type, std::move(encoded),
                                               line);
}

void __inspector_var_update_enum(const char* name, void* addr,
                           const inspector::TypeDescriptor* type, long long value) {
    inspector::EncodedValue encoded = inspector::TraceState::instance().encodeEnum(value, type);
    inspector::TraceState::instance().recordVarUpdate(name, addr, type, std::move(encoded));
}

void __inspector_var_init_union(const char* name, void* addr,
                          const inspector::TypeDescriptor* type, int line) {
    inspector::EncodedValue value = inspector::TraceState::instance().encodeUnion(addr, type);
    inspector::TraceState::instance().recordVarInit(name, addr, type, std::move(value),
                                               line);
}

void __inspector_var_update_union(const char* name, void* addr,
                            const inspector::TypeDescriptor* type) {
    inspector::EncodedValue value = inspector::TraceState::instance().encodeUnion(addr, type);
    inspector::TraceState::instance().recordVarUpdate(name, addr, type, std::move(value));
}

void __inspector_var_init_array(const char* name, void* addr,
                          const inspector::TypeDescriptor* type, int line) {
    inspector::EncodedValue value = inspector::TraceState::instance().encodeArray(addr, type);
    inspector::TraceState::instance().recordVarInit(name, addr, type, std::move(value),
                                               line);
}

void __inspector_var_update_array(const char* name, void* addr,
                            const inspector::TypeDescriptor* type) {
    inspector::EncodedValue value = inspector::TraceState::instance().encodeArray(addr, type);
    inspector::TraceState::instance().recordVarUpdate(name, addr, type, std::move(value));
}

// ---------------------------------------------------------------------------
// Tier 3: Heap allocation tracking
// ---------------------------------------------------------------------------

int __inspector_alloc(void* ptr, unsigned long size, const inspector::TypeDescriptor* type,
                int is_array, unsigned long array_count) {
    return inspector::TraceState::instance().recordAlloc(
        ptr, static_cast<size_t>(size), type,
        is_array != 0, static_cast<size_t>(array_count));
}

void __inspector_dealloc(void* ptr) {
    inspector::TraceState::instance().recordFree(ptr);
}

// ---------------------------------------------------------------------------
// Tier 3: malloc/free shim hooks (C-style allocations without type info)
// ---------------------------------------------------------------------------

void __inspector_alloc_malloc(void* ptr, size_t size) {
    // Record allocation with no type information
    inspector::TraceState::instance().recordAlloc(
        ptr, size, nullptr, false, 1);
}

void __inspector_dealloc_malloc(void* ptr) {
    inspector::TraceState::instance().recordFree(ptr);
}

// --- Legacy compatibility (single int-only hook) ---

void __inspector_var_init(const char* name, void* addr, int value) {
    __inspector_var_init_int(name, addr, &inspector::TYPE_INT, value, 0);
}

void __inspector_var_update(const char* name, void* addr, int value) {
    __inspector_var_update_int(name, addr, &inspector::TYPE_INT, value);
}

} // extern "C"
