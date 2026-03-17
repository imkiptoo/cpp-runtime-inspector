//! @file see/TypeInfo.cpp
//! @brief Type descriptor implementations.

#include "TypeInfo.h"

namespace see {

const char* typeKindToString(TypeKind kind) {
    switch (kind) {
    case TypeKind::Int:
        return "int";
    case TypeKind::UInt:
        return "uint";
    case TypeKind::Float:
        return "float";
    case TypeKind::Bool:
        return "bool";
    case TypeKind::Char:
        return "char";
    case TypeKind::Pointer:
        return "pointer";
    case TypeKind::Reference:
        return "reference";
    case TypeKind::Struct:
        return "struct";
    case TypeKind::Array:
        return "array";
    case TypeKind::Enum:
        return "enum";
    case TypeKind::Union:
        return "union";
    case TypeKind::Void:
        return "void";
    case TypeKind::Unknown:
    default:
        return "unknown";
    }
}

const char* accessLevelToString(AccessLevel level) {
    switch (level) {
    case AccessLevel::Public:
        return "public";
    case AccessLevel::Protected:
        return "protected";
    case AccessLevel::Private:
        return "private";
    default:
        return "unknown";
    }
}

const char* lookupEnumName(const TypeDescriptor* type, long long value) {
    if (!type || type->kind != TypeKind::Enum || !type->enum_values)
        return nullptr;

    // Binary search for the value
    size_t left = 0;
    size_t right = type->enum_value_count;
    while (left < right) {
        size_t mid = left + (right - left) / 2;
        if (type->enum_values[mid].value == value) {
            return type->enum_values[mid].name;
        } else if (type->enum_values[mid].value < value) {
            left = mid + 1;
        } else {
            right = mid;
        }
    }
    return nullptr;
}

// Helper macro for defining built-in type descriptors
// TypeDescriptor: kind, spelling, size, fields, field_count, element_type,
//                 element_count, bases, base_count, enum_values, enum_value_count,
//                 is_scoped_enum, is_polymorphic, is_union
#define BUILTIN_TYPE(k, s, sz) \
    {k, s, sz, nullptr, 0, nullptr, 0, nullptr, 0, nullptr, 0, false, false, false}

#define BUILTIN_PTR(s, elem) \
    {TypeKind::Pointer, s, sizeof(void*), nullptr, 0, elem, 0, nullptr, 0, nullptr, 0, false, false, false}

// Built-in type descriptors
const TypeDescriptor TYPE_INT = BUILTIN_TYPE(TypeKind::Int, "int", sizeof(int));
const TypeDescriptor TYPE_UINT = BUILTIN_TYPE(TypeKind::UInt, "unsigned int", sizeof(unsigned int));
const TypeDescriptor TYPE_LONG = BUILTIN_TYPE(TypeKind::Int, "long", sizeof(long));
const TypeDescriptor TYPE_ULONG = BUILTIN_TYPE(TypeKind::UInt, "unsigned long", sizeof(unsigned long));
const TypeDescriptor TYPE_LLONG = BUILTIN_TYPE(TypeKind::Int, "long long", sizeof(long long));
const TypeDescriptor TYPE_ULLONG = BUILTIN_TYPE(TypeKind::UInt, "unsigned long long", sizeof(unsigned long long));
const TypeDescriptor TYPE_FLOAT = BUILTIN_TYPE(TypeKind::Float, "float", sizeof(float));
const TypeDescriptor TYPE_DOUBLE = BUILTIN_TYPE(TypeKind::Float, "double", sizeof(double));
const TypeDescriptor TYPE_BOOL = BUILTIN_TYPE(TypeKind::Bool, "bool", sizeof(bool));
const TypeDescriptor TYPE_CHAR = BUILTIN_TYPE(TypeKind::Char, "char", sizeof(char));
const TypeDescriptor TYPE_UCHAR = BUILTIN_TYPE(TypeKind::Char, "unsigned char", sizeof(unsigned char));
const TypeDescriptor TYPE_PTR_CHAR = BUILTIN_PTR("char*", &TYPE_CHAR);
const TypeDescriptor TYPE_PTR_VOID = BUILTIN_PTR("void*", nullptr);

#undef BUILTIN_TYPE
#undef BUILTIN_PTR

} // namespace see
