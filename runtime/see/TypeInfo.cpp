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

// Built-in type descriptors
const TypeDescriptor TYPE_INT = {TypeKind::Int, "int", sizeof(int),
                                  nullptr, 0, nullptr, 0};
const TypeDescriptor TYPE_UINT = {TypeKind::UInt, "unsigned int",
                                   sizeof(unsigned int), nullptr, 0, nullptr, 0};
const TypeDescriptor TYPE_LONG = {TypeKind::Int, "long", sizeof(long),
                                   nullptr, 0, nullptr, 0};
const TypeDescriptor TYPE_ULONG = {TypeKind::UInt, "unsigned long",
                                    sizeof(unsigned long), nullptr, 0, nullptr, 0};
const TypeDescriptor TYPE_LLONG = {TypeKind::Int, "long long", sizeof(long long),
                                    nullptr, 0, nullptr, 0};
const TypeDescriptor TYPE_ULLONG = {TypeKind::UInt, "unsigned long long",
                                     sizeof(unsigned long long), nullptr, 0,
                                     nullptr, 0};
const TypeDescriptor TYPE_FLOAT = {TypeKind::Float, "float", sizeof(float),
                                    nullptr, 0, nullptr, 0};
const TypeDescriptor TYPE_DOUBLE = {TypeKind::Float, "double", sizeof(double),
                                     nullptr, 0, nullptr, 0};
const TypeDescriptor TYPE_BOOL = {TypeKind::Bool, "bool", sizeof(bool),
                                   nullptr, 0, nullptr, 0};
const TypeDescriptor TYPE_CHAR = {TypeKind::Char, "char", sizeof(char),
                                   nullptr, 0, nullptr, 0};
const TypeDescriptor TYPE_UCHAR = {TypeKind::Char, "unsigned char",
                                    sizeof(unsigned char), nullptr, 0, nullptr, 0};
const TypeDescriptor TYPE_PTR_CHAR = {TypeKind::Pointer, "char*", sizeof(char*),
                                       nullptr, 0, &TYPE_CHAR, 0};
const TypeDescriptor TYPE_PTR_VOID = {TypeKind::Pointer, "void*", sizeof(void*),
                                       nullptr, 0, nullptr, 0};

} // namespace see
