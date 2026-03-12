//! @file see/TypeInfo.h
//! @brief Type descriptor definitions for the See++ runtime.

#pragma once

#include <cstddef>
#include <cstdint>

namespace see {

//! Type categories for runtime dispatch.
enum class TypeKind : uint8_t {
    Int,       //!< Signed integer types
    UInt,      //!< Unsigned integer types
    Float,     //!< Floating point types
    Bool,      //!< Boolean type
    Char,      //!< Character types
    Pointer,   //!< Pointer types
    Reference, //!< Reference types
    // Tier 2:
    Struct, //!< Struct/class types
    Array,  //!< Array types
    Enum,   //!< Enumeration types
    Union,  //!< Union types
    // Special:
    Void,   //!< Void type
    Unknown //!< Unsupported types
};

//! Forward declaration for recursive type references.
struct TypeDescriptor;

//! Field information for struct types (Tier 2).
struct FieldInfo {
    const char* name;
    size_t offset;
    const TypeDescriptor* type;
};

//! Describes a type for runtime serialization.
//!
//! Generated as static data by the plugin for each unique type encountered.
struct TypeDescriptor {
    TypeKind kind;          //!< Type category
    const char* spelling;   //!< Human-readable type name
    size_t size;            //!< sizeof(T)

    // For struct types (Tier 2):
    const FieldInfo* fields;
    size_t field_count;

    // For array types (Tier 2):
    const TypeDescriptor* element_type;
    size_t element_count;
};

//! Convert TypeKind to string for JSON output.
const char* typeKindToString(TypeKind kind);

//! Built-in type descriptors for common types.
extern const TypeDescriptor TYPE_INT;
extern const TypeDescriptor TYPE_UINT;
extern const TypeDescriptor TYPE_LONG;
extern const TypeDescriptor TYPE_ULONG;
extern const TypeDescriptor TYPE_LLONG;
extern const TypeDescriptor TYPE_ULLONG;
extern const TypeDescriptor TYPE_FLOAT;
extern const TypeDescriptor TYPE_DOUBLE;
extern const TypeDescriptor TYPE_BOOL;
extern const TypeDescriptor TYPE_CHAR;
extern const TypeDescriptor TYPE_UCHAR;
extern const TypeDescriptor TYPE_PTR_CHAR;    // char*
extern const TypeDescriptor TYPE_PTR_VOID;    // void*

} // namespace see
