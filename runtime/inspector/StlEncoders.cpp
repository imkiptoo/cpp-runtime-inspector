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

    if (typeName.find("std::variant") != std::string::npos ||
        typeName.find("std::__1::variant") != std::string::npos) {
        return StlContainerKind::Variant;
    }

    if (typeName.find("std::function") != std::string::npos ||
        typeName.find("std::__1::function") != std::string::npos) {
        return StlContainerKind::Function;
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

EncodedValue encodeStdMap(const void* addr, const TypeDescriptor* mapType,
                          const TypeDescriptor* elementType, bool isSet,
                          TraceState& state) {
    // libstdc++ x86_64 std::map / std::set layout:
    //
    //   class _Rb_tree {
    //     _Rb_tree_impl _M_impl;       // contains _Rb_tree_header
    //   };
    //
    //   _Rb_tree_header is at offset 8 within map/set (the leading 8 bytes
    //   appear to come from a non-EBO'd allocator slot in this build), so:
    //
    //     offset (size-32): root pointer (_M_parent)
    //     offset (size-24): leftmost node (_M_left)
    //     offset (size-16): rightmost node (_M_right)
    //     offset (size-8):  size_t count
    //
    //   Each node lives in a _Rb_tree_node which begins with a 32-byte
    //   _Rb_tree_node_base (4 color + 4 pad + 8 parent + 8 left + 8 right)
    //   followed by the stored value at offset 32.
    constexpr size_t NODE_BASE_SIZE = 32;
    constexpr size_t MAP_FOOTER_OFFSET = 32; // count is at the very end (-8)

    StructValue sv;
    sv.typeName = isSet ? "std::set" : "std::map";
    sv.fieldOrder.push_back("size");
    sv.fieldOrder.push_back("entries");

    auto sizeHolder = std::make_shared<EncodedValueHolder>();
    auto entriesHolder = std::make_shared<EncodedValueHolder>();

    if (!addr || !mapType || mapType->size < 40) {
        sizeHolder->value = static_cast<long long>(0);
        ArrayValue av;
        av.elements.clear();
        entriesHolder->value = av;
        sv.fields["size"] = sizeHolder;
        sv.fields["entries"] = entriesHolder;
        return sv;
    }

    const auto* base = reinterpret_cast<const unsigned char*>(addr);
    size_t totalSize = mapType->size;
    const void* root = *reinterpret_cast<const void* const*>(base + totalSize - MAP_FOOTER_OFFSET);
    size_t count = *reinterpret_cast<const size_t*>(base + totalSize - 8);

    sizeHolder->value = static_cast<long long>(count);
    sv.fields["size"] = sizeHolder;

    ArrayValue av;
    av.elementTypeName = elementType && elementType->spelling
                            ? elementType->spelling
                            : "<unknown>";

    // In-order tree walk. Stop early at large sizes so we can never blow up
    // the trace if the tree is corrupted.
    constexpr size_t MAX_ENTRIES = 256;
    size_t emitted = 0;

    // Iterative in-order traversal using parent pointers (libstdc++ stores
    // parent in _M_parent at offset +8 of node base).
    auto leftOf = [](const void* n) -> const void* {
        return *reinterpret_cast<const void* const*>(reinterpret_cast<const unsigned char*>(n) + 16);
    };
    auto rightOf = [](const void* n) -> const void* {
        return *reinterpret_cast<const void* const*>(reinterpret_cast<const unsigned char*>(n) + 24);
    };
    auto parentOf = [](const void* n) -> const void* {
        return *reinterpret_cast<const void* const*>(reinterpret_cast<const unsigned char*>(n) + 8);
    };

    if (root && count > 0 && elementType) {
        // Find leftmost from root.
        const void* node = root;
        while (leftOf(node)) node = leftOf(node);

        const void* headerEnd = base + totalSize - MAP_FOOTER_OFFSET;
        while (node && emitted < count && emitted < MAX_ENTRIES) {
            const void* valueAddr = reinterpret_cast<const unsigned char*>(node) + NODE_BASE_SIZE;
            auto holder = std::make_shared<EncodedValueHolder>();
            holder->type = elementType;
            holder->value = state.encodeValue(valueAddr, elementType);
            av.elements.push_back(holder);
            ++emitted;

            // In-order successor.
            if (rightOf(node)) {
                node = rightOf(node);
                while (leftOf(node)) node = leftOf(node);
            } else {
                const void* p = parentOf(node);
                while (p && rightOf(p) == node) {
                    node = p;
                    p = parentOf(node);
                    if (p == headerEnd) { p = nullptr; break; }
                }
                if (p == headerEnd) break;
                node = p;
            }
        }
    }

    entriesHolder->value = av;
    sv.fields["entries"] = entriesHolder;
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
    // libstdc++ / libc++ on Linux lay out optional<T> as
    //   union { char _M_empty; T _M_payload; } + bool _M_engaged;
    // so the engaged flag sits at offset sizeof(T) (bool aligns to 1).
    // We only inspect the engaged byte and the payload at offset 0.
    StructValue sv;
    sv.typeName = "std::optional";
    sv.fieldOrder.push_back("has_value");
    sv.fieldOrder.push_back("value");

    if (!addr || !valueType || valueType->size == 0) {
        auto eng = std::make_shared<EncodedValueHolder>();
        eng->value = false;
        sv.fields["has_value"] = eng;
        auto val = std::make_shared<EncodedValueHolder>();
        val->value = std::string("<unknown payload type>");
        sv.fields["value"] = val;
        return sv;
    }

    const auto* base = reinterpret_cast<const unsigned char*>(addr);
    bool engaged = base[valueType->size] != 0;

    auto eng = std::make_shared<EncodedValueHolder>();
    eng->value = engaged;
    sv.fields["has_value"] = eng;

    auto val = std::make_shared<EncodedValueHolder>();
    if (engaged) {
        val->type = valueType;
        val->value = state.encodeValue(addr, valueType);
    } else {
        val->value = std::string("<empty>");
    }
    sv.fields["value"] = val;

    return sv;
}

EncodedValue encodeStdVariant(const void* addr, size_t variantSize,
                              const TypeDescriptor* firstAlternativeType,
                              TraceState& state) {
    StructValue sv;
    sv.typeName = "std::variant";
    sv.fieldOrder.push_back("index");
    sv.fieldOrder.push_back("value");

    if (!addr || variantSize < 9) {
        auto idx = std::make_shared<EncodedValueHolder>();
        idx->value = static_cast<long long>(-1);
        sv.fields["index"] = idx;
        auto val = std::make_shared<EncodedValueHolder>();
        val->value = std::string("<invalid variant>");
        sv.fields["value"] = val;
        return sv;
    }

    // libstdc++ Linux: discriminator is a small unsigned integer in the last
    // alignof(variant)==8 byte slot. The lowest byte holds the index for any
    // variant with <=255 alternatives.
    const auto* base = reinterpret_cast<const unsigned char*>(addr);
    size_t discOffset = variantSize - 8;
    long long index = static_cast<long long>(base[discOffset]);
    // valueless_by_exception() returns 0xff for "no value".
    if (base[discOffset] == 0xff) index = -1;

    auto idx = std::make_shared<EncodedValueHolder>();
    idx->value = index;
    sv.fields["index"] = idx;

    auto val = std::make_shared<EncodedValueHolder>();
    if (index == 0 && firstAlternativeType) {
        val->type = firstAlternativeType;
        val->value = state.encodeValue(addr, firstAlternativeType);
    } else if (index < 0) {
        val->value = std::string("<valueless>");
    } else {
        val->value = std::string("<alternative beyond first; payload not introspectable>");
    }
    sv.fields["value"] = val;
    return sv;
}

EncodedValue encodeStdFunction(const void* addr) {
    StructValue sv;
    sv.typeName = "std::function";
    sv.fieldOrder.push_back("engaged");

    if (!addr) {
        auto eng = std::make_shared<EncodedValueHolder>();
        eng->value = false;
        sv.fields["engaged"] = eng;
        return sv;
    }

    // libstdc++ std::function stores _M_invoker (a function pointer) and
    // _M_manager (another function pointer). Both are nullptr for an empty
    // std::function. We just need to know whether either is non-null.
    const void* const* slots = reinterpret_cast<const void* const*>(addr);
    bool engaged = slots[2] != nullptr || slots[3] != nullptr;
    auto eng = std::make_shared<EncodedValueHolder>();
    eng->value = engaged;
    sv.fields["engaged"] = eng;
    return sv;
}

const TypeDescriptor* extractElementType(const TypeDescriptor* containerType) {
    if (!containerType) return nullptr;

    // For containers, element_type should be set
    return containerType->element_type;
}

} // namespace inspector
