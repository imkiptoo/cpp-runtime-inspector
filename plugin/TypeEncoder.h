//! @file TypeEncoder.h
//! @brief Type descriptor generation for the See++ plugin.
//!
//! This module handles encoding of C++ types into TypeDescriptor structures
//! that can be emitted as static data in instrumented code.

#pragma once

#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/Type.h"

#include <set>
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

//! Access level for struct/class members (matches runtime).
enum class AccessLevel : uint8_t {
    Public,
    Protected,
    Private
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

    //! Check if a type is a struct/class type.
    bool isStruct(clang::QualType type) const;

    //! Check if a type is an enum type.
    bool isEnum(clang::QualType type) const;

    //! Check if a type is a union type.
    bool isUnion(clang::QualType type) const;

    //! Check if a type is a fixed-size array type.
    bool isArray(clang::QualType type) const;

    //! Get the element type and count for array types.
    std::pair<clang::QualType, size_t> getArrayInfo(clang::QualType type) const;

    //! Generate complete TypeDescriptor code for a type, including all
    //! dependencies (field types, base types, etc.).
    //! Returns all necessary declarations in topological order.
    std::string generateTypeDescriptorCode(clang::QualType type);

    //! Check if we've already generated a descriptor for this type.
    bool hasGeneratedDescriptor(clang::QualType type) const;

    //! Mark a type as having its descriptor generated.
    void markDescriptorGenerated(clang::QualType type);

    //! Get the reference name for a type descriptor variable.
    std::string getDescriptorRef(clang::QualType type) const;

private:
    clang::ASTContext& m_context;

    //! Track types we've already generated descriptors for.
    mutable std::set<std::string> m_generatedTypes;

    //! Convert TypeKind to string for code generation.
    static const char* kindToString(TypeKind kind);

    //! Convert AccessLevel to string for code generation.
    static const char* accessToString(AccessLevel level);

    //! Convert Clang access specifier to our AccessLevel.
    static AccessLevel convertAccess(clang::AccessSpecifier spec);

    //! Generate FieldInfo array code for a record type.
    std::string generateFieldInfoArray(const clang::RecordDecl* record,
                                        const std::string& baseName);

    //! Generate EnumValue array code for an enum type.
    std::string generateEnumValueArray(const clang::EnumDecl* decl,
                                        const std::string& baseName);

    //! Generate BaseInfo array code for a CXXRecord with bases.
    std::string generateBaseInfoArray(const clang::CXXRecordDecl* record,
                                       const std::string& baseName);
};

} // namespace see
