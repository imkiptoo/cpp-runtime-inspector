//! @file TypeEncoder.h
//! @brief Type descriptor generation for the See++ plugin.
//!
//! This module handles encoding of C++ types into TypeDescriptor structures
//! that can be emitted as static data in instrumented code.

#pragma once

#include "clang/AST/ASTContext.h"
#include "clang/AST/Type.h"

#include <string>
#include <unordered_map>

namespace see {

//! Type kind categories matching runtime TypeKind enum.
enum class TypeKind : uint8_t {
    Int,
    UInt,
    Float,
    Bool,
    Char,
    Pointer,
    Reference,
    // Tier 2 additions:
    Struct,
    Array,
    Enum,
    Union,
    // Special cases:
    Void,
    Unknown
};

//! Encodes C++ types into See++ type descriptors.
class TypeEncoder {
public:
    explicit TypeEncoder(clang::ASTContext& context);

    //! Get the TypeKind for a Clang QualType.
    TypeKind getTypeKind(clang::QualType type) const;

    //! Get a mangled name suitable for use in identifiers.
    std::string getMangledName(clang::QualType type) const;

    //! Get the human-readable type spelling.
    std::string getSpelling(clang::QualType type) const;

    //! Get the size of a type in bytes.
    size_t getSize(clang::QualType type) const;

    //! Check if a type is a primitive (int, float, bool, char).
    bool isPrimitive(clang::QualType type) const;

    //! Check if a type is a pointer type.
    bool isPointer(clang::QualType type) const;

    //! Check if a type is a reference type.
    bool isReference(clang::QualType type) const;

    //! Check if a type is a C-string type (const char*, char*).
    bool isCString(clang::QualType type) const;

    //! Get the pointee type for pointer/reference types.
    clang::QualType getPointeeType(clang::QualType type) const;

    //! Generate static TypeDescriptor declaration code.
    std::string generateDescriptor(clang::QualType type) const;

    //! Get the runtime hook function name suffix for a type.
    //! Returns "int", "uint", "float", "bool", "char", "ptr", "ref".
    std::string getHookSuffix(clang::QualType type) const;

    //! Check if a type is supported for instrumentation.
    bool isSupported(clang::QualType type) const;

private:
    clang::ASTContext& m_context;

    //! Convert TypeKind to string for code generation.
    static const char* kindToString(TypeKind kind);
};

} // namespace see
