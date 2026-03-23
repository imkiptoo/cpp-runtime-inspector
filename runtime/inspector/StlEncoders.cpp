//! @file inspector/StlEncoders.cpp
//! @brief Implementation of STL container encoders.
//!
//! WARNING: This code reaches into libstdc++ internals and is FRAGILE.
//! It is pinned to libstdc++ as shipped with GCC 11-13 on Linux.
//! Different library versions may have different internal layouts.

#include "StlEncoders.h"

#include <cstring>
#include <regex>

namespace inspector {

// ---------------------------------------------------------------------------
// Container identification
// ---------------------------------------------------------------------------

StlContainerKind identifyStlContainer(const std::string& typeName) {
    // Use simple string matching instead of regex for performance
    // Check for common STL container patterns

    if (typeName.find("std::vector") != std::string::npos ||
        typeName.find("std::__1::vector") != std::string::npos) {
        return StlContainerKind::Vector;
    }

    if (typeName.find("std::basic_string") != std::string::npos ||
        typeName.find("std::string") != std::string::npos ||
        typeName.find("std::__1::basic_string") != std::string::npos) {
        return StlContainerKind::String;
    }

    if (typeName.find("std::array") != std::string::npos ||
        typeName.find("std::__1::array") != std::string::npos) {
        return StlContainerKind::Array;
    }

    if (typeName.find("std::pair") != std::string::npos ||
        typeName.find("std::__1::pair") != std::string::npos) {
        return StlContainerKind::Pair;
    }

    if (typeName.find("std::map") != std::string::npos ||
        typeName.find("std::__1::map") != std::string::npos) {
        return StlContainerKind::Map;
    }

    if (typeName.find("std::set") != std::string::npos ||
        typeName.find("std::__1::set") != std::string::npos) {
        return StlContainerKind::Set;
    }

    if (typeName.find("std::unique_ptr") != std::string::npos ||
        typeName.find("std::__1::unique_ptr") != std::string::npos) {
        return StlContainerKind::UniquePtr;
    }

    if (typeName.find("std::shared_ptr") != std::string::npos ||
        typeName.find("std::__1::shared_ptr") != std::string::npos) {
        return StlContainerKind::SharedPtr;
    }

    if (typeName.find("std::optional") != std::string::npos ||
        typeName.find("std::__1::optional") != std::string::npos) {
        return StlContainerKind::Optional;
    }

    return StlContainerKind::None;
}

// ---------------------------------------------------------------------------
// Encoder implementations
// ---------------------------------------------------------------------------

EncodedValue encodeStdVector(const void* addr, const TypeDescriptor* elementType,
                             TraceState& state) {
    ArrayValue av;
    av.elementTypeName = elementType ? (elementType->spelling ? elementType->spelling : "unknown") : "unknown";

    if (!addr) {
        return av;  // Empty array
    }

    // Cast to internal representation
    // Layout: _M_start (8 bytes), _M_finish (8 bytes), _M_end_of_storage (8 bytes)
    const char* base = static_cast<const char*>(addr);
    const void* const* ptrs = reinterpret_cast<const void* const*>(base);
    const char* start = static_cast<const char*>(ptrs[0]);
    const char* finish = static_cast<const char*>(ptrs[1]);

    if (!start || !finish || start >= finish) {
        return av;  // Empty or uninitialized vector
    }

    // Calculate number of elements
    size_t elementSize = elementType ? elementType->size : sizeof(void*);
    if (elementSize == 0) elementSize = 1;  // Prevent division by zero

    size_t count = static_cast<size_t>(finish - start) / elementSize;

    // Cap at reasonable limit to prevent huge traces
    const size_t MAX_ELEMENTS = 100;
    if (count > MAX_ELEMENTS) count = MAX_ELEMENTS;

    // Encode each element
    for (size_t i = 0; i < count; ++i) {
        const void* elemAddr = start + (i * elementSize);
        auto holder = std::make_shared<EncodedValueHolder>();
        holder->type = elementType;
        holder->value = state.encodeValue(elemAddr, elementType);
        av.elements.push_back(holder);
    }

    return av;
}

EncodedValue encodeStdString(const void* addr) {
    if (!addr) {
        return std::string("");
    }

    // Try to safely read string contents
    // NOTE: This is fragile - the internal layout varies between implementations
    // libstdc++ (GCC): pointer to data, then length
    // libc++ (LLVM/macOS): different layout with SSO

    // For safety, try to use the std::string directly if we can cast it
    // This works because we know addr points to a valid std::string
    try {
        const std::string* str = reinterpret_cast<const std::string*>(addr);

        // Cap length for safety
        const size_t MAX_STRING_LENGTH = 256;
        size_t length = str->length();
        if (length > MAX_STRING_LENGTH) {
            return str->substr(0, MAX_STRING_LENGTH) + "...";
        }
        return *str;
    } catch (...) {
        return std::string("<string read error>");
    }
}

EncodedValue encodeStdArray(const void* addr, const TypeDescriptor* elementType,
                            size_t size, TraceState& state) {
    ArrayValue av;
    av.elementTypeName = elementType ? (elementType->spelling ? elementType->spelling : "unknown") : "unknown";

    if (!addr || size == 0) {
        return av;
    }

    size_t elementSize = elementType ? elementType->size : sizeof(void*);
    if (elementSize == 0) elementSize = 1;

    const char* base = static_cast<const char*>(addr);

    // Cap at reasonable limit
    const size_t MAX_ELEMENTS = 100;
    size_t count = (size > MAX_ELEMENTS) ? MAX_ELEMENTS : size;

    for (size_t i = 0; i < count; ++i) {
        const void* elemAddr = base + (i * elementSize);
        auto holder = std::make_shared<EncodedValueHolder>();
        holder->type = elementType;
        holder->value = state.encodeValue(elemAddr, elementType);
        av.elements.push_back(holder);
    }

    return av;
}

EncodedValue encodeStdPair(const void* addr, const TypeDescriptor* firstType,
                           const TypeDescriptor* secondType, TraceState& state) {
    StructValue sv;
    sv.typeName = "std::pair";

    if (!addr) {
        return sv;
    }

    const char* base = static_cast<const char*>(addr);

    // First element is at offset 0
    sv.fieldOrder.push_back("first");
    auto firstHolder = std::make_shared<EncodedValueHolder>();
    firstHolder->type = firstType;
    firstHolder->value = state.encodeValue(base, firstType);
    sv.fields["first"] = firstHolder;

    // Second element is at offset sizeof(first) with alignment
    size_t firstSize = firstType ? firstType->size : sizeof(void*);
    size_t secondAlign = secondType ? secondType->size : sizeof(void*);
    if (secondAlign > 8) secondAlign = 8;  // Common alignment cap

    // Calculate aligned offset for second
    size_t secondOffset = (firstSize + secondAlign - 1) & ~(secondAlign - 1);

    sv.fieldOrder.push_back("second");
    auto secondHolder = std::make_shared<EncodedValueHolder>();
    secondHolder->type = secondType;
    secondHolder->value = state.encodeValue(base + secondOffset, secondType);
    sv.fields["second"] = secondHolder;

    return sv;
}

EncodedValue encodeStdMap(const void* addr, const TypeDescriptor* keyType,
                          const TypeDescriptor* valueType, TraceState& state) {
    // std::map uses a red-black tree internally
    // This is very complex to traverse without the type information
    // For now, return a placeholder indicating it's a map
    StructValue sv;
    sv.typeName = "std::map";
    sv.fieldOrder.push_back("<contents>");

    auto holder = std::make_shared<EncodedValueHolder>();
    holder->value = std::string("<map traversal not yet implemented>");
    sv.fields["<contents>"] = holder;

    return sv;
}

EncodedValue encodeStdUniquePtr(const void* addr, const TypeDescriptor* pointeeType,
                                TraceState& state) {
    if (!addr) {
        // Return null pointer encoding
        std::vector<std::string> result;
        result.push_back("C_ADDRESS");
        result.push_back("0x0");
        result.push_back(pointeeType && pointeeType->spelling ? pointeeType->spelling : "void*");
        result.push_back("null");
        return result;
    }

    // unique_ptr stores the pointer directly (with deleter, but that's usually empty)
    const void* const* ptrToPtr = reinterpret_cast<const void* const*>(addr);
    const void* ptr = *ptrToPtr;

    if (!ptr) {
        std::vector<std::string> result;
        result.push_back("C_ADDRESS");
        result.push_back("0x0");
        result.push_back(pointeeType && pointeeType->spelling ? pointeeType->spelling : "void*");
        result.push_back("null");
        return result;
    }

    // Encode as a pointer, potentially resolving to heap
    return state.encodePointer(ptr, pointeeType);
}

EncodedValue encodeStdSharedPtr(const void* addr, const TypeDescriptor* pointeeType,
                                TraceState& state) {
    if (!addr) {
        std::vector<std::string> result;
        result.push_back("C_ADDRESS");
        result.push_back("0x0");
        result.push_back(pointeeType && pointeeType->spelling ? pointeeType->spelling : "void*");
        result.push_back("null");
        return result;
    }

    // shared_ptr has: T* _M_ptr, __shared_count<> _M_refcount
    const char* base = static_cast<const char*>(addr);
    const void* const* ptrToPtr = reinterpret_cast<const void* const*>(base);
    const void* ptr = *ptrToPtr;

    if (!ptr) {
        std::vector<std::string> result;
        result.push_back("C_ADDRESS");
        result.push_back("0x0");
        result.push_back(pointeeType && pointeeType->spelling ? pointeeType->spelling : "void*");
        result.push_back("null");
        return result;
    }

    return state.encodePointer(ptr, pointeeType);
}

EncodedValue encodeStdOptional(const void* addr, const TypeDescriptor* valueType,
                               TraceState& state) {
    // std::optional layout varies by implementation
    // For now, return a simplified representation
    StructValue sv;
    sv.typeName = "std::optional";
    sv.fieldOrder.push_back("<value>");

    auto holder = std::make_shared<EncodedValueHolder>();
    holder->value = std::string("<optional inspection not yet implemented>");
    sv.fields["<value>"] = holder;

    return sv;
}

const TypeDescriptor* extractElementType(const TypeDescriptor* containerType) {
    if (!containerType) return nullptr;

    // For containers, element_type should be set
    return containerType->element_type;
}

} // namespace inspector
