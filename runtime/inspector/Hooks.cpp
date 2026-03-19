//! @file see/Hooks.cpp
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
    see::ensureExitHandlerRegistered();
    see::TraceState::instance().pushFrame(funcName ? funcName : "<null>", line);
}

void __inspector_leave(const char* funcName, int line) {
    (void)funcName; // Used for validation in debug builds
    see::TraceState::instance().popFrame(line);
}

void __inspector_step(int line) {
    see::TraceState::instance().recordStep(line);
}

// --- Integer types ---

void __inspector_var_init_int(const char* name, void* addr,
                        const see::TypeDescriptor* type, long long value,
                        int line) {
    see::TraceState::instance().recordVarInit(
        name, addr, type, see::EncodedValue{value}, line);
}

void __inspector_var_update_int(const char* name, void* addr,
                          const see::TypeDescriptor* type, long long value) {
    see::TraceState::instance().recordVarUpdate(name, addr, type,
                                                 see::EncodedValue{value});
}

void __inspector_var_init_uint(const char* name, void* addr,
                         const see::TypeDescriptor* type,
                         unsigned long long value, int line) {
    see::TraceState::instance().recordVarInit(
        name, addr, type, see::EncodedValue{value}, line);
}

void __inspector_var_update_uint(const char* name, void* addr,
                           const see::TypeDescriptor* type,
                           unsigned long long value) {
    see::TraceState::instance().recordVarUpdate(name, addr, type,
                                                 see::EncodedValue{value});
}

// --- Floating point types ---

void __inspector_var_init_float(const char* name, void* addr,
                          const see::TypeDescriptor* type, double value,
                          int line) {
    see::TraceState::instance().recordVarInit(
        name, addr, type, see::EncodedValue{value}, line);
}

void __inspector_var_update_float(const char* name, void* addr,
                            const see::TypeDescriptor* type, double value) {
    see::TraceState::instance().recordVarUpdate(name, addr, type,
                                                 see::EncodedValue{value});
}

// --- Boolean type ---

void __inspector_var_init_bool(const char* name, void* addr,
                         const see::TypeDescriptor* type, bool value,
                         int line) {
    see::TraceState::instance().recordVarInit(
        name, addr, type, see::EncodedValue{value}, line);
}

void __inspector_var_update_bool(const char* name, void* addr,
                           const see::TypeDescriptor* type, bool value) {
    see::TraceState::instance().recordVarUpdate(name, addr, type,
                                                 see::EncodedValue{value});
}

// --- Character type ---

void __inspector_var_init_char(const char* name, void* addr,
                         const see::TypeDescriptor* type, int value,
                         int line) {
    see::TraceState::instance().recordVarInit(
        name, addr, type, see::EncodedValue{static_cast<char>(value)}, line);
}

void __inspector_var_update_char(const char* name, void* addr,
                           const see::TypeDescriptor* type, int value) {
    see::TraceState::instance().recordVarUpdate(
        name, addr, type, see::EncodedValue{static_cast<char>(value)});
}

// --- Pointer types ---

void __inspector_var_init_ptr(const char* name, void* addr,
                        const see::TypeDescriptor* type, const void* ptr_value,
                        int line) {
    see::EncodedValue value = see::encodePointerValue(ptr_value, type);

    // Special handling for C-strings
    if (type && type->element_type &&
        type->element_type->kind == see::TypeKind::Char) {
        std::string str = see::safeReadString(static_cast<const char*>(ptr_value), 256);
        if (!str.empty()) {
            value = str;
        }
    }

    see::TraceState::instance().recordVarInit(name, addr, type, std::move(value),
                                               line);
}

void __inspector_var_update_ptr(const char* name, void* addr,
                          const see::TypeDescriptor* type,
                          const void* ptr_value) {
    see::EncodedValue value = see::encodePointerValue(ptr_value, type);

    // Special handling for C-strings
    if (type && type->element_type &&
        type->element_type->kind == see::TypeKind::Char) {
        std::string str = see::safeReadString(static_cast<const char*>(ptr_value), 256);
        if (!str.empty()) {
            value = str;
        }
    }

    see::TraceState::instance().recordVarUpdate(name, addr, type,
                                                 std::move(value));
}

// --- Reference types ---

void __inspector_var_init_ref(const char* name, void* addr,
                        const see::TypeDescriptor* type,
                        const void* referent_addr, int line) {
    see::EncodedValue value = see::encodePointerValue(referent_addr, type);
    see::TraceState::instance().recordVarInit(name, addr, type, std::move(value),
                                               line);
}

void __inspector_var_update_ref(const char* name, void* addr,
                          const see::TypeDescriptor* type,
                          const void* referent_addr) {
    see::EncodedValue value = see::encodePointerValue(referent_addr, type);
    see::TraceState::instance().recordVarUpdate(name, addr, type,
                                                 std::move(value));
}

// ---------------------------------------------------------------------------
// Tier 2: Composite types
// ---------------------------------------------------------------------------

void __inspector_var_init_struct(const char* name, void* addr,
                           const see::TypeDescriptor* type, int line) {
    see::EncodedValue value = see::TraceState::instance().encodeStruct(addr, type);
    see::TraceState::instance().recordVarInit(name, addr, type, std::move(value),
                                               line);
}

void __inspector_var_update_struct(const char* name, void* addr,
                             const see::TypeDescriptor* type) {
    see::EncodedValue value = see::TraceState::instance().encodeStruct(addr, type);
    see::TraceState::instance().recordVarUpdate(name, addr, type, std::move(value));
}

void __inspector_var_init_enum(const char* name, void* addr,
                         const see::TypeDescriptor* type, long long value,
                         int line) {
    see::EncodedValue encoded = see::TraceState::instance().encodeEnum(value, type);
    see::TraceState::instance().recordVarInit(name, addr, type, std::move(encoded),
                                               line);
}

void __inspector_var_update_enum(const char* name, void* addr,
                           const see::TypeDescriptor* type, long long value) {
    see::EncodedValue encoded = see::TraceState::instance().encodeEnum(value, type);
    see::TraceState::instance().recordVarUpdate(name, addr, type, std::move(encoded));
}

void __inspector_var_init_union(const char* name, void* addr,
                          const see::TypeDescriptor* type, int line) {
    see::EncodedValue value = see::TraceState::instance().encodeUnion(addr, type);
    see::TraceState::instance().recordVarInit(name, addr, type, std::move(value),
                                               line);
}

void __inspector_var_update_union(const char* name, void* addr,
                            const see::TypeDescriptor* type) {
    see::EncodedValue value = see::TraceState::instance().encodeUnion(addr, type);
    see::TraceState::instance().recordVarUpdate(name, addr, type, std::move(value));
}

void __inspector_var_init_array(const char* name, void* addr,
                          const see::TypeDescriptor* type, int line) {
    see::EncodedValue value = see::TraceState::instance().encodeArray(addr, type);
    see::TraceState::instance().recordVarInit(name, addr, type, std::move(value),
                                               line);
}

void __inspector_var_update_array(const char* name, void* addr,
                            const see::TypeDescriptor* type) {
    see::EncodedValue value = see::TraceState::instance().encodeArray(addr, type);
    see::TraceState::instance().recordVarUpdate(name, addr, type, std::move(value));
}

// ---------------------------------------------------------------------------
// Tier 3: Heap allocation tracking
// ---------------------------------------------------------------------------

int __inspector_alloc(void* ptr, unsigned long size, const see::TypeDescriptor* type,
                int is_array, unsigned long array_count) {
    return see::TraceState::instance().recordAlloc(
        ptr, static_cast<size_t>(size), type,
        is_array != 0, static_cast<size_t>(array_count));
}

void __inspector_dealloc(void* ptr) {
    see::TraceState::instance().recordFree(ptr);
}

// --- Legacy compatibility (single int-only hook) ---

void __inspector_var_init(const char* name, void* addr, int value) {
    __inspector_var_init_int(name, addr, &see::TYPE_INT, value, 0);
}

void __inspector_var_update(const char* name, void* addr, int value) {
    __inspector_var_update_int(name, addr, &see::TYPE_INT, value);
}

} // extern "C"
