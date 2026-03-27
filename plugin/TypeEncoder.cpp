//! @file TypeEncoder.cpp
//! @brief Implementation of type descriptor generation.

#include "TypeEncoder.h"
#include "clang/AST/Mangle.h"
#include "clang/AST/RecordLayout.h"

#include <algorithm>
#include <sstream>

namespace inspector {

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

    // Check enum BEFORE integer types, as unscoped enums are considered
    // integer types by Clang
    if (type->isEnumeralType())
        return TypeKind::Enum;

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

    if (type->isArrayType())
        return TypeKind::Array;

    // Handle both struct and class types (they're equivalent in C++)
    if (type->isStructureType() || type->isClassType())
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

    ss << "static const inspector::TypeDescriptor __inspector_type_" << mangledName
       << " = {\n";
    ss << "    inspector::TypeKind::" << kindToString(kind) << ",\n";
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
    case TypeKind::Struct:
        return "struct";
    case TypeKind::Array:
        return "array";
    case TypeKind::Enum:
        return "enum";
    case TypeKind::Union:
        return "union";
    default:
        return "generic";
    }
}

bool TypeEncoder::isSupported(clang::QualType type) const {
    TypeKind kind = getTypeKind(type);
    if (kind == TypeKind::Unknown || kind == TypeKind::Void)
        return false;

    // For Tier 2, we support all basic composite types
    // Check if the type is complete for record types
    type = type.getCanonicalType();
    if (type->isRecordType()) {
        const clang::RecordDecl* record = type->getAsRecordDecl();
        if (!record || !record->isCompleteDefinition())
            return false;
    }

    return true;
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

const char* TypeEncoder::accessToString(AccessLevel level) {
    switch (level) {
    case AccessLevel::Public:
        return "Public";
    case AccessLevel::Protected:
        return "Protected";
    case AccessLevel::Private:
        return "Private";
    default:
        return "Public";
    }
}

AccessLevel TypeEncoder::convertAccess(clang::AccessSpecifier spec) {
    switch (spec) {
    case clang::AS_public:
        return AccessLevel::Public;
    case clang::AS_protected:
        return AccessLevel::Protected;
    case clang::AS_private:
        return AccessLevel::Private;
    default:
        return AccessLevel::Public;
    }
}

bool TypeEncoder::isStruct(clang::QualType type) const {
    type = type.getCanonicalType();
    return type->isStructureType() || type->isClassType();
}

bool TypeEncoder::isEnum(clang::QualType type) const {
    type = type.getCanonicalType();
    return type->isEnumeralType();
}

bool TypeEncoder::isUnion(clang::QualType type) const {
    type = type.getCanonicalType();
    return type->isUnionType();
}

bool TypeEncoder::isArray(clang::QualType type) const {
    type = type.getCanonicalType();
    return type->isConstantArrayType();
}

std::pair<clang::QualType, size_t>
TypeEncoder::getArrayInfo(clang::QualType type) const {
    type = type.getCanonicalType();
    if (const auto* arr = llvm::dyn_cast<clang::ConstantArrayType>(type)) {
        return {arr->getElementType(), arr->getSize().getZExtValue()};
    }
    return {clang::QualType(), 0};
}

bool TypeEncoder::isStlContainer(clang::QualType type) const {
    type = type.getCanonicalType();
    if (!type->isRecordType())
        return false;

    std::string typeName = type.getAsString();

    // Check for common STL container patterns
    if (typeName.find("std::vector") != std::string::npos ||
        typeName.find("std::string") != std::string::npos ||
        typeName.find("std::basic_string") != std::string::npos ||
        typeName.find("std::array") != std::string::npos ||
        typeName.find("std::pair") != std::string::npos ||
        typeName.find("std::map") != std::string::npos ||
        typeName.find("std::set") != std::string::npos ||
        typeName.find("std::unique_ptr") != std::string::npos ||
        typeName.find("std::shared_ptr") != std::string::npos ||
        typeName.find("std::optional") != std::string::npos) {
        return true;
    }

    return false;
}

bool TypeEncoder::isLambda(clang::QualType type) const {
    type = type.getCanonicalType();
    if (!type->isRecordType())
        return false;

    const clang::RecordDecl* record = type->getAsRecordDecl();
    if (!record)
        return false;

    // Lambda classes have names like "(lambda at file.cpp:line:col)"
    if (const auto* cxxRecord = llvm::dyn_cast<clang::CXXRecordDecl>(record)) {
        return cxxRecord->isLambda();
    }

    return false;
}

std::string TypeEncoder::getLambdaFriendlyName(clang::QualType type) const {
    // Track lambda count for numbering
    static std::unordered_map<std::string, int> lambdaCounters;
    static int globalLambdaCounter = 0;

    type = type.getCanonicalType();
    std::string fullName = type.getAsString();

    // Extract file and line info if possible
    // Lambda names look like: (lambda at /path/to/file.cpp:line:col)
    size_t atPos = fullName.find(" at ");
    if (atPos != std::string::npos) {
        // Use a simple counter based on the full name
        auto it = lambdaCounters.find(fullName);
        if (it == lambdaCounters.end()) {
            lambdaCounters[fullName] = ++globalLambdaCounter;
        }
        return "<lambda#" + std::to_string(lambdaCounters[fullName]) + ">";
    }

    return "<lambda>";
}

clang::QualType TypeEncoder::getStlElementType(clang::QualType type) const {
    type = type.getCanonicalType();

    // For template types, get the first template argument
    if (const auto* tst = type->getAs<clang::TemplateSpecializationType>()) {
        if (tst->template_arguments().size() > 0) {
            const auto& arg = tst->template_arguments()[0];
            if (arg.getKind() == clang::TemplateArgument::Type) {
                return arg.getAsType();
            }
        }
    }

    // Try to get from record decl for instantiated templates
    if (const auto* record = type->getAsCXXRecordDecl()) {
        if (const auto* spec = llvm::dyn_cast<clang::ClassTemplateSpecializationDecl>(record)) {
            const auto& args = spec->getTemplateArgs();
            if (args.size() > 0 && args[0].getKind() == clang::TemplateArgument::Type) {
                return args[0].getAsType();
            }
        }
    }

    return clang::QualType();
}

bool TypeEncoder::hasGeneratedDescriptor(clang::QualType type) const {
    return m_generatedTypes.count(getMangledName(type)) > 0;
}

void TypeEncoder::markDescriptorGenerated(clang::QualType type) {
    m_generatedTypes.insert(getMangledName(type));
}

std::string TypeEncoder::getDescriptorRef(clang::QualType type) const {
    // For builtin types, use the predefined constants
    type = type.getCanonicalType();
    if (type->isBuiltinType()) {
        const auto* builtin = type->getAs<clang::BuiltinType>();
        switch (builtin->getKind()) {
        case clang::BuiltinType::Int:
            return "&inspector::TYPE_INT";
        case clang::BuiltinType::UInt:
            return "&inspector::TYPE_UINT";
        case clang::BuiltinType::Long:
            return "&inspector::TYPE_LONG";
        case clang::BuiltinType::ULong:
            return "&inspector::TYPE_ULONG";
        case clang::BuiltinType::LongLong:
            return "&inspector::TYPE_LLONG";
        case clang::BuiltinType::ULongLong:
            return "&inspector::TYPE_ULLONG";
        case clang::BuiltinType::Float:
            return "&inspector::TYPE_FLOAT";
        case clang::BuiltinType::Double:
            return "&inspector::TYPE_DOUBLE";
        case clang::BuiltinType::Bool:
            return "&inspector::TYPE_BOOL";
        case clang::BuiltinType::Char_S:
        case clang::BuiltinType::Char_U:
        case clang::BuiltinType::SChar:
            return "&inspector::TYPE_CHAR";
        case clang::BuiltinType::UChar:
            return "&inspector::TYPE_UCHAR";
        default:
            break;
        }
    }

    // For pointer to char, use predefined
    if (type->isPointerType()) {
        clang::QualType pointee = type->getPointeeType();
        if (pointee->isCharType())
            return "&inspector::TYPE_PTR_CHAR";
        if (pointee->isVoidType())
            return "&inspector::TYPE_PTR_VOID";
    }

    return "&__inspector_type_" + getMangledName(type);
}

std::string
TypeEncoder::generateFieldInfoArray(const clang::RecordDecl* record,
                                     const std::string& baseName) {
    if (!record || !record->isCompleteDefinition())
        return "";

    std::ostringstream ss;
    const clang::ASTRecordLayout& layout =
        m_context.getASTRecordLayout(record);

    // Collect fields
    std::vector<std::tuple<std::string, size_t, clang::QualType, AccessLevel>>
        fields;

    unsigned fieldIdx = 0;
    for (const auto* field : record->fields()) {
        size_t offset = layout.getFieldOffset(fieldIdx) / 8;
        AccessLevel access = AccessLevel::Public;

        // For C++ classes, get access specifier
        if (const auto* cxxRecord =
                llvm::dyn_cast<clang::CXXRecordDecl>(record)) {
            access = convertAccess(field->getAccess());
        }

        fields.emplace_back(field->getNameAsString(), offset, field->getType(),
                            access);
        ++fieldIdx;
    }

    if (fields.empty())
        return "";

    ss << "static const inspector::FieldInfo " << baseName << "_fields[] = {\n";
    for (size_t i = 0; i < fields.size(); ++i) {
        const auto& [name, offset, type, access] = fields[i];
        ss << "    {\"" << name << "\", " << offset << ", "
           << getDescriptorRef(type) << ", inspector::AccessLevel::"
           << accessToString(access) << ", false}";
        if (i + 1 < fields.size())
            ss << ",";
        ss << "\n";
    }
    ss << "};\n";

    return ss.str();
}

std::string
TypeEncoder::generateEnumValueArray(const clang::EnumDecl* decl,
                                     const std::string& baseName) {
    if (!decl)
        return "";

    std::ostringstream ss;
    std::vector<std::pair<std::string, long long>> values;

    for (const auto* enumerator : decl->enumerators()) {
        values.emplace_back(enumerator->getNameAsString(),
                            enumerator->getInitVal().getExtValue());
    }

    // Sort by value for binary search in runtime
    std::sort(values.begin(), values.end(),
              [](const auto& a, const auto& b) { return a.second < b.second; });

    if (values.empty())
        return "";

    ss << "static const inspector::EnumValue " << baseName << "_values[] = {\n";
    for (size_t i = 0; i < values.size(); ++i) {
        ss << "    {" << values[i].second << ", \"" << values[i].first << "\"}";
        if (i + 1 < values.size())
            ss << ",";
        ss << "\n";
    }
    ss << "};\n";

    return ss.str();
}

std::string
TypeEncoder::generateBaseInfoArray(const clang::CXXRecordDecl* record,
                                    const std::string& baseName) {
    if (!record || !record->getNumBases())
        return "";

    std::ostringstream ss;
    const clang::ASTRecordLayout& layout =
        m_context.getASTRecordLayout(record);

    ss << "static const inspector::BaseInfo " << baseName << "_bases[] = {\n";

    unsigned idx = 0;
    for (const auto& base : record->bases()) {
        clang::QualType baseType = base.getType();
        size_t offset = 0;
        bool isVirtual = base.isVirtual();

        if (!isVirtual) {
            const auto* baseDecl = baseType->getAsCXXRecordDecl();
            if (baseDecl) {
                offset = layout.getBaseClassOffset(baseDecl).getQuantity();
            }
        }

        ss << "    {" << getDescriptorRef(baseType) << ", " << offset << ", "
           << (isVirtual ? "true" : "false") << "}";
        if (++idx < record->getNumBases())
            ss << ",";
        ss << "\n";
    }
    ss << "};\n";

    return ss.str();
}

std::string TypeEncoder::generateTypeDescriptorCode(clang::QualType type) {
    type = type.getCanonicalType();

    // Skip builtin types - they have predefined descriptors in the runtime
    if (type->isBuiltinType()) {
        return "";
    }

    // Skip pointer to char/void - also predefined
    if (type->isPointerType()) {
        clang::QualType pointee = type->getPointeeType();
        if (pointee->isCharType() || pointee->isVoidType()) {
            return "";
        }
    }

    std::string mangledName = getMangledName(type);

    // Skip if already fully generated
    if (hasGeneratedDescriptor(type))
        return "";

    // Detect circular reference - if this type is in progress, just emit extern declaration
    if (m_inProgressTypes.count(mangledName) > 0) {
        return "";  // Definition will come later
    }

    // Mark as in-progress
    m_inProgressTypes.insert(mangledName);

    std::ostringstream ss;
    std::ostringstream externDecls;  // Extern declarations for circular refs
    TypeKind kind = getTypeKind(type);
    std::string spelling = getSpelling(type);
    size_t size = getSize(type);

    // For record types, check if any field types need extern declarations
    // This handles circular references (e.g., struct Node { Node* next; })
    if (type->isRecordType()) {
        const clang::RecordDecl* record = type->getAsRecordDecl();
        if (record && record->isCompleteDefinition()) {
            for (const auto* field : record->fields()) {
                clang::QualType fieldType = field->getType().getCanonicalType();
                if (fieldType->isPointerType() || fieldType->isReferenceType()) {
                    clang::QualType pointee = fieldType->getPointeeType();
                    if (pointee->isCharType() || pointee->isVoidType()) {
                        continue;
                    }
                    std::string fieldMangledName = getMangledName(fieldType);
                    // If this field type is in-progress (circular ref), emit extern declaration
                    if (m_inProgressTypes.count(fieldMangledName) > 0 &&
                        !hasGeneratedDescriptor(fieldType)) {
                        externDecls << "extern const inspector::TypeDescriptor __inspector_type_"
                                    << fieldMangledName << ";\n";
                    }
                }
            }
        }
    }

    // For composite types, first generate dependency descriptors
    std::string dependencies;

    if (type->isRecordType()) {
        const clang::RecordDecl* record = type->getAsRecordDecl();
        if (record && record->isCompleteDefinition()) {
            // Generate descriptors for field types
            for (const auto* field : record->fields()) {
                dependencies += generateTypeDescriptorCode(field->getType());
            }

            // Generate descriptors for base classes
            if (const auto* cxxRecord =
                    llvm::dyn_cast<clang::CXXRecordDecl>(record)) {
                for (const auto& base : cxxRecord->bases()) {
                    dependencies += generateTypeDescriptorCode(base.getType());
                }
            }
        }
    }

    if (type->isConstantArrayType()) {
        auto [elemType, count] = getArrayInfo(type);
        dependencies += generateTypeDescriptorCode(elemType);
    }

    if (type->isPointerType() || type->isReferenceType()) {
        clang::QualType pointee = getPointeeType(type);
        if (!pointee.isNull() && !pointee->isVoidType()) {
            dependencies += generateTypeDescriptorCode(pointee);
        }
    }

    // Output extern declarations for circular references
    ss << externDecls.str();
    ss << dependencies;

    // Generate auxiliary arrays for composite types
    std::string fieldArrayName = "__inspector_type_" + mangledName;
    std::string fieldArrayCode;
    std::string enumArrayCode;
    std::string baseArrayCode;
    size_t fieldCount = 0;
    size_t enumValueCount = 0;
    size_t baseCount = 0;
    bool isScopedEnum = false;
    bool isPolymorphic = false;
    bool isUnionType = type->isUnionType();

    if (type->isRecordType()) {
        const clang::RecordDecl* record = type->getAsRecordDecl();
        if (record && record->isCompleteDefinition()) {
            fieldArrayCode =
                generateFieldInfoArray(record, fieldArrayName);
            fieldCount = std::distance(record->field_begin(), record->field_end());

            if (const auto* cxxRecord =
                    llvm::dyn_cast<clang::CXXRecordDecl>(record)) {
                isPolymorphic = cxxRecord->isPolymorphic();
                if (cxxRecord->getNumBases() > 0) {
                    baseArrayCode =
                        generateBaseInfoArray(cxxRecord, fieldArrayName);
                    baseCount = cxxRecord->getNumBases();
                }
            }
        }
    }

    if (type->isEnumeralType()) {
        const clang::EnumDecl* enumDecl = type->getAs<clang::EnumType>()->getDecl();
        if (enumDecl) {
            enumArrayCode = generateEnumValueArray(enumDecl, fieldArrayName);
            enumValueCount =
                std::distance(enumDecl->enumerator_begin(), enumDecl->enumerator_end());
            isScopedEnum = enumDecl->isScoped();
        }
    }

    ss << fieldArrayCode;
    ss << enumArrayCode;
    ss << baseArrayCode;

    // Generate the TypeDescriptor
    // Use 'const' instead of 'static const' to allow extern declarations for circular refs
    ss << "const inspector::TypeDescriptor __inspector_type_" << mangledName
       << " = {\n";
    ss << "    inspector::TypeKind::" << kindToString(kind) << ",\n";
    ss << "    \"" << spelling << "\",\n";
    ss << "    " << size << ",\n";

    // Fields
    if (fieldCount > 0) {
        ss << "    " << fieldArrayName << "_fields, " << fieldCount << ",\n";
    } else {
        ss << "    nullptr, 0,\n";
    }

    // Element type (for arrays and pointers)
    if (type->isConstantArrayType()) {
        auto [elemType, count] = getArrayInfo(type);
        ss << "    " << getDescriptorRef(elemType) << ", " << count << ",\n";
    } else if (type->isPointerType() || type->isReferenceType()) {
        clang::QualType pointee = getPointeeType(type);
        if (!pointee.isNull() && !pointee->isVoidType()) {
            ss << "    " << getDescriptorRef(pointee) << ", 0,\n";
        } else {
            ss << "    nullptr, 0,\n";
        }
    } else {
        ss << "    nullptr, 0,\n";
    }

    // Base classes
    if (baseCount > 0) {
        ss << "    " << fieldArrayName << "_bases, " << baseCount << ",\n";
    } else {
        ss << "    nullptr, 0,\n";
    }

    // Enum values
    if (enumValueCount > 0) {
        ss << "    " << fieldArrayName << "_values, " << enumValueCount << ",\n";
    } else {
        ss << "    nullptr, 0,\n";
    }

    ss << "    " << (isScopedEnum ? "true" : "false") << ",\n";
    ss << "    " << (isPolymorphic ? "true" : "false") << ",\n";
    ss << "    " << (isUnionType ? "true" : "false") << "\n";
    ss << "};\n\n";

    // Mark as fully generated and remove from in-progress
    markDescriptorGenerated(type);
    m_inProgressTypes.erase(mangledName);

    return ss.str();
}

} // namespace inspector
