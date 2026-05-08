//! @file inspector/StlEncoders.h
//! @brief Custom encoders for STL containers.
//!
//! These encoders reach into libstdc++ internals to extract the logical
//! contents of STL containers. This code is fragile and pinned to specific
//! library versions.
//!
//! IMPORTANT: This module is designed for libstdc++ (GCC). libc++ (LLVM)
//! has different internal layouts and is not supported.

#pragma once

#include "TypeInfo.h"
#include "Trace.h"

#include <cstddef>
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace inspector {

// TraceState is defined in Trace.h

//! Identifies which STL container type this is.
enum class StlContainerKind {
    None,           //!< Not an STL container
    Vector,         //!< std::vector<T>
    String,         //!< std::string / std::basic_string<char>
    Array,          //!< std::array<T, N>
    Pair,           //!< std::pair<T1, T2>
    Map,            //!< std::map<K, V>
    Set,            //!< std::set<T>
    UniquePtr,      //!< std::unique_ptr<T>
    SharedPtr,      //!< std::shared_ptr<T>
    Optional,       //!< std::optional<T>
    Variant,        //!< std::variant<Ts...>
    Function,       //!< std::function<R(Args...)>
};

//! Check if a type name is an STL container we can encode.
//! @param typeName The canonical type name (e.g., "std::vector<int>").
//! @return The container kind, or None if not recognized.
StlContainerKind identifyStlContainer(const std::string& typeName);

//! Encode an std::vector's contents.
//! @param addr Address of the vector object.
//! @param elementType Type descriptor for vector elements.
//! @param state TraceState for recursive encoding.
//! @return Encoded value representing the vector contents.
EncodedValue encodeStdVector(const void* addr, const TypeDescriptor* elementType,
                             TraceState& state);

//! Encode an std::string's contents.
//! @param addr Address of the string object.
//! @return Encoded value (the string contents or error indicator).
EncodedValue encodeStdString(const void* addr);

//! Encode an std::array's contents.
//! @param addr Address of the array object.
//! @param elementType Type descriptor for array elements.
//! @param size Number of elements in the array.
//! @param state TraceState for recursive encoding.
//! @return Encoded value representing the array contents.
EncodedValue encodeStdArray(const void* addr, const TypeDescriptor* elementType,
                            size_t size, TraceState& state);

//! Encode an std::pair's contents.
//! @param addr Address of the pair object.
//! @param firstType Type descriptor for first element.
//! @param secondType Type descriptor for second element.
//! @param state TraceState for recursive encoding.
//! @return Encoded value representing the pair.
EncodedValue encodeStdPair(const void* addr, const TypeDescriptor* firstType,
                           const TypeDescriptor* secondType, TraceState& state);

//! Encode an std::map's contents.
//! @param addr Address of the map object.
//! @param keyType Type descriptor for keys.
//! @param valueType Type descriptor for values.
//! @param state TraceState for recursive encoding.
//! @return Encoded value representing the map contents.
EncodedValue encodeStdMap(const void* addr, const TypeDescriptor* mapType,
                          const TypeDescriptor* elementType, bool isSet,
                          TraceState& state);

//! Encode an std::unique_ptr's contents.
//! @param addr Address of the unique_ptr object.
//! @param pointeeType Type descriptor for the pointed-to type.
//! @param state TraceState for recursive encoding.
//! @return Encoded value representing the pointer.
EncodedValue encodeStdUniquePtr(const void* addr, const TypeDescriptor* pointeeType,
                                TraceState& state);

//! Encode an std::shared_ptr's contents.
//! @param addr Address of the shared_ptr object.
//! @param pointeeType Type descriptor for the pointed-to type.
//! @param state TraceState for recursive encoding.
//! @return Encoded value representing the pointer and ref count.
EncodedValue encodeStdSharedPtr(const void* addr, const TypeDescriptor* pointeeType,
                                TraceState& state);

//! Encode an std::optional's contents.
//! @param addr Address of the optional object.
//! @param valueType Type descriptor for the contained type.
//! @param state TraceState for recursive encoding.
//! @return Encoded value representing the optional (engaged or empty).
EncodedValue encodeStdOptional(const void* addr, const TypeDescriptor* valueType,
                               TraceState& state);

//! Encode a std::variant.
//! Reports the runtime index plus, for index 0 only, the decoded payload
//! using `firstAlternativeType`. Other alternatives are listed by raw
//! storage hex because their TypeDescriptors are not currently emitted.
//! @param addr Address of the variant object.
//! @param variantSize sizeof(variant) — used to locate the discriminator.
//! @param firstAlternativeType Type descriptor for the first template argument.
EncodedValue encodeStdVariant(const void* addr, size_t variantSize,
                              const TypeDescriptor* firstAlternativeType,
                              TraceState& state);

//! Encode a std::function.
//! Reports whether the target is engaged (non-empty) and, for libstdc++,
//! the rough target representation. Layout-pinned to libstdc++ Linux.
EncodedValue encodeStdFunction(const void* addr);

//! Extract element type descriptor from an STL container type descriptor.
//! For vector<T>, returns the descriptor for T.
//! @param containerType Type descriptor for the container.
//! @return Type descriptor for the element, or nullptr if not available.
const TypeDescriptor* extractElementType(const TypeDescriptor* containerType);

} // namespace inspector
