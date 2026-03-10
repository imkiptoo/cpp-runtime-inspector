//! @file TypeEncoder.cpp
//! @brief Implementation of type descriptor generation.

#include "TypeEncoder.h"
#include "clang/AST/Mangle.h"

#include <sstream>

namespace see {

TypeEncoder::TypeEncoder(clang::ASTContext& context) : m_context(context) {}

TypeKind TypeEncoder::getTypeKind(clang::QualType type) const {
    // Remove qualifiers for classification
    type = type.getCanonicalType();

    if (type->isVoidType())
        return TypeKind::Void;

    if (type->isReferenceType())
        return TypeKind::Reference;

    if (type->isPointerType())
        return TypeKind::Pointer;

    if (type->isBooleanType())
        return TypeKind::Bool;

    if (type->isCharType())
        return TypeKind::Char;

    if (type->isFloatingType())
        return TypeKind::Float;

    if (type->isUnsignedIntegerType())
        return TypeKind::UInt;

    if (type->isSignedIntegerType())
        return TypeKind::Int;

    if (type->isEnumeralType())
        return TypeKind::Enum;

    if (type->isArrayType())
        return TypeKind::Array;

    if (type->isStructureType())
        return TypeKind::Struct;

    if (type->isUnionType())
        return TypeKind::Union;

    return TypeKind::Unknown;
}

std::string TypeEncoder::getMangledName(clang::QualType type) const {
    std::string result;
    llvm::raw_string_ostream os(result);

    std::unique_ptr<clang::MangleContext> mangler(
        m_context.createMangleContext());

    // For builtin types, create a simple name
    type = type.getCanonicalType();
    if (type->isBuiltinType()) {
        const auto* builtin = type->getAs<clang::BuiltinType>();
        switch (builtin->getKind()) {
        case clang::BuiltinType::Bool:
            return "bool";
        case clang::BuiltinType::Char_S:
        case clang::BuiltinType::Char_U:
        case clang::BuiltinType::SChar:
            return "char";
        case clang::BuiltinType::UChar:
            return "uchar";
        case clang::BuiltinType::Short:
            return "short";
        case clang::BuiltinType::UShort:
            return "ushort";
        case clang::BuiltinType::Int:
            return "int";
        case clang::BuiltinType::UInt:
            return "uint";
        case clang::BuiltinType::Long:
            return "long";
        case clang::BuiltinType::ULong:
            return "ulong";
        case clang::BuiltinType::LongLong:
            return "llong";
        case clang::BuiltinType::ULongLong:
            return "ullong";
        case clang::BuiltinType::Float:
            return "float";
        case clang::BuiltinType::Double:
            return "double";
        case clang::BuiltinType::LongDouble:
            return "ldouble";
        default:
            break;
        }
    }

    // For pointer types
    if (type->isPointerType()) {
        std::string pointee = getMangledName(type->getPointeeType());
        return "ptr_" + pointee;
    }

    // For reference types
    if (type->isReferenceType()) {
        std::string referent =
            getMangledName(type.getNonReferenceType());
        return "ref_" + referent;
    }

    // Fallback: use type spelling with invalid chars replaced
    std::string spelling = type.getAsString();
    for (char& c : spelling) {
        if (!std::isalnum(c))
            c = '_';
    }
    return spelling;
}

std::string TypeEncoder::getSpelling(clang::QualType type) const {
    return type.getAsString();
}

size_t TypeEncoder::getSize(clang::QualType type) const {
    if (type->isIncompleteType())
        return 0;
    return m_context.getTypeSize(type) / 8;
}

bool TypeEncoder::isPrimitive(clang::QualType type) const {
    TypeKind kind = getTypeKind(type);
    return kind == TypeKind::Int || kind == TypeKind::UInt ||
           kind == TypeKind::Float || kind == TypeKind::Bool ||
           kind == TypeKind::Char;
}

bool TypeEncoder::isPointer(clang::QualType type) const {
    return type->isPointerType();
}

bool TypeEncoder::isReference(clang::QualType type) const {
    return type->isReferenceType();
}

bool TypeEncoder::isCString(clang::QualType type) const {
    if (!type->isPointerType())
        return false;

    clang::QualType pointee = type->getPointeeType();
    return pointee->isCharType();
}

clang::QualType TypeEncoder::getPointeeType(clang::QualType type) const {
    if (type->isPointerType())
        return type->getPointeeType();
    if (type->isReferenceType())
        return type.getNonReferenceType();
    return clang::QualType();
}

std::string TypeEncoder::generateDescriptor(clang::QualType type) const {
    std::ostringstream ss;

    std::string mangledName = getMangledName(type);
    std::string spelling = getSpelling(type);
    size_t size = getSize(type);
    TypeKind kind = getTypeKind(type);

    ss << "static const see::TypeDescriptor __see_type_" << mangledName
       << " = {\n";
    ss << "    see::TypeKind::" << kindToString(kind) << ",\n";
    ss << "    \"" << spelling << "\",\n";
    ss << "    " << size << ",\n";
    ss << "    nullptr, 0,  // fields\n";
    ss << "    nullptr, 0   // element\n";
    ss << "};\n";

    return ss.str();
}

std::string TypeEncoder::getHookSuffix(clang::QualType type) const {
    TypeKind kind = getTypeKind(type);
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
        return "ptr";
    case TypeKind::Reference:
        return "ref";
    default:
        return "generic";
    }
}

bool TypeEncoder::isSupported(clang::QualType type) const {
    TypeKind kind = getTypeKind(type);
    return kind != TypeKind::Unknown && kind != TypeKind::Void;
}

const char* TypeEncoder::kindToString(TypeKind kind) {
    switch (kind) {
    case TypeKind::Int:
        return "Int";
    case TypeKind::UInt:
        return "UInt";
    case TypeKind::Float:
        return "Float";
    case TypeKind::Bool:
        return "Bool";
    case TypeKind::Char:
        return "Char";
    case TypeKind::Pointer:
        return "Pointer";
    case TypeKind::Reference:
        return "Reference";
    case TypeKind::Struct:
        return "Struct";
    case TypeKind::Array:
        return "Array";
    case TypeKind::Enum:
        return "Enum";
    case TypeKind::Union:
        return "Union";
    case TypeKind::Void:
        return "Void";
    case TypeKind::Unknown:
    default:
        return "Unknown";
    }
}

} // namespace see
