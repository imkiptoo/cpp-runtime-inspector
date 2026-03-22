//! @file inspector/TypeInfo.h
//! @brief Type descriptor definitions for the C++ Runtime Inspector runtime.

#pragma once

#include <cstddef>
#include <cstdint>

namespace inspector {

//! Type categories for runtime dispatch.
enum class TypeKind : uint8_t {
    Int,       //!< Signed integer types
    UInt,      //!< Unsigned integer types
    Float,     //!< Floating point types
    Bool,      //!< Boolean type
    Char,      //!< Character types
    Pointer,   //!< Pointer types
    Reference, //!< Reference types
    Struct,    //!< Struct/class types
    Array,     //!< Array types
    Enum,      //!< Enumeration types
    Union,     //!< Union types
    Void,      //!< Void type
    Unknown    //!< Unsupported types
};

//! Access level for struct/class members.
enum class AccessLevel : uint8_t {
    Public,
    Protected,
    Private
};

//! Forward declaration for recursive type references.
struct TypeDescriptor;

//! Field information for struct/class/union types.
struct FieldInfo {
    const char* name;             //!< Field name
    size_t offset;                //!< Byte offset from struct base
    const TypeDescriptor* type;   //!< Field type descriptor
    AccessLevel access;           //!< Access level (public/protected/private)
    bool is_vptr;                 //!< True if this is a vptr field
};

//! Base class information for inheritance.
struct BaseInfo {
    const TypeDescriptor* type;   //!< Base class type descriptor
    size_t offset;                //!< Byte offset (usually 0 for single inheritance)
    bool is_virtual;              //!< True if virtual inheritance
};

//! Enum value-name pair for enum types.
struct EnumValue {
    long long value;              //!< Enum underlying value
    const char* name;             //!< Enum enumerator name
};

//! Describes a type for runtime serialization.
//!
//! Generated as static data by the plugin for each unique type encountered.
struct TypeDescriptor {
    TypeKind kind;                //!< Type category
    const char* spelling;         //!< Human-readable type name
    size_t size;                  //!< sizeof(T)

    // For struct/class/union types:
    const FieldInfo* fields;      //!< Array of field descriptors
    size_t field_count;           //!< Number of fields

    // For array types:
    const TypeDescriptor* element_type;  //!< Element type descriptor
    size_t element_count;                //!< Number of elements (0 for dynamic)

    // For class types with inheritance:
    const BaseInfo* bases;        //!< Array of base class descriptors
    size_t base_count;            //!< Number of base classes

    // For enum types:
    const EnumValue* enum_values; //!< Array of enum value-name pairs
    size_t enum_value_count;      //!< Number of enum values
    bool is_scoped_enum;          //!< True if enum class

    // Flags:
    bool is_polymorphic;          //!< Has virtual functions
    bool is_union;                //!< Is a union (fields overlap)
};

//! Convert TypeKind to string for JSON output.
const char* typeKindToString(TypeKind kind);

//! Convert AccessLevel to string for JSON output.
const char* accessLevelToString(AccessLevel level);

//! Look up enum value name by value. Returns nullptr if not found.
const char* lookupEnumName(const TypeDescriptor* type, long long value);

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

} // namespace inspector
