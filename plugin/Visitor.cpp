//! @file Visitor.cpp
//! @brief Implementation of the C++ Runtime Inspector AST visitor.

#include "Visitor.h"

#include "clang/AST/ParentMapContext.h"
#include "clang/Lex/Lexer.h"

#include <sstream>

namespace inspector {

InspectorVisitor::InspectorVisitor(clang::Rewriter& rewriter, clang::ASTContext& context)
    : m_helpers(rewriter, context), m_typeEncoder(context), m_context(context),
      m_rewriter(rewriter) {}

void InspectorVisitor::finalize() {
    // Flush any pending type descriptors
    if (m_hasInsertionPoint && !m_pendingDescriptors.empty()) {
        m_rewriter.InsertTextBefore(m_insertionPoint,
                                    "// C++ Runtime Inspector type descriptors\n" +
                                        m_pendingDescriptors + "\n");
        m_pendingDescriptors.clear();
    }
}

bool InspectorVisitor::TraverseStmt(clang::Stmt* stmt) {
    if (!stmt)
        return true;

    m_parentStack.push_back(stmt);
    bool result = clang::RecursiveASTVisitor<InspectorVisitor>::TraverseStmt(stmt);
    m_parentStack.pop_back();
    return result;
}

bool InspectorVisitor::hasCompoundStmtParent() const {
    // The current statement is at the back, its parent is second-to-last
    if (m_parentStack.size() < 2)
        return false;
    return llvm::isa<clang::CompoundStmt>(
        m_parentStack[m_parentStack.size() - 2]);
}

bool InspectorVisitor::VisitFunctionDecl(clang::FunctionDecl* decl) {
    if (!decl->hasBody() || !m_helpers.isInMainFile(decl->getLocation()))
        return true;

    clang::Stmt* body = decl->getBody();
    auto* compound = llvm::dyn_cast<clang::CompoundStmt>(body);
    if (!compound)
        return true;

    std::string funcName = decl->getNameAsString();
    m_currentFunction = funcName;

    // Use the first top-level function as the insertion point for type descriptors
    // Skip member functions (methods) - we only want free functions
    if (!m_hasInsertionPoint && !decl->isCXXClassMember()) {
        m_insertionPoint = decl->getBeginLoc();
        m_hasInsertionPoint = true;
    }

    // Inject __inspector_enter after opening brace
    unsigned enterLine = m_helpers.getLineNumber(compound->getLBracLoc());
    std::string enterCall =
        "__inspector_enter(\"" + funcName + "\", " + std::to_string(enterLine) + "); ";
    m_rewriter.InsertTextAfterToken(compound->getLBracLoc(),
                                    "\n    " + enterCall);

    // Inject __inspector_leave before closing brace
    unsigned leaveLine = m_helpers.getLineNumber(compound->getRBracLoc());
    std::string leaveCall =
        "    __inspector_leave(\"" + funcName + "\", " + std::to_string(leaveLine) +
        ");\n";
    m_rewriter.InsertTextBefore(compound->getRBracLoc(), leaveCall);

    return true;
}

bool InspectorVisitor::VisitReturnStmt(clang::ReturnStmt* stmt) {
    if (!m_helpers.isInMainFile(stmt->getBeginLoc()))
        return true;

    std::string funcName = findEnclosingFunctionName(stmt);
    unsigned line = m_helpers.getLineNumber(stmt->getBeginLoc());
    std::string call =
        "__inspector_leave(\"" + funcName + "\", " + std::to_string(line) + "); ";
    m_rewriter.InsertTextBefore(stmt->getBeginLoc(), call);

    return true;
}

bool InspectorVisitor::VisitVarDecl(clang::VarDecl* decl) {
    if (!m_helpers.isInMainFile(decl->getLocation()))
        return true;

    // Only instrument local variables
    if (!decl->isLocalVarDecl())
        return true;

    // Skip exception catch clause declarations
    if (decl->isExceptionVariable())
        return true;

    // Skip variables declared in for-loop init expressions
    // These cannot be instrumented because inserting text after them breaks the for syntax
    const auto& parents = m_context.getParents(*decl);
    for (const auto& parent : parents) {
        if (const auto* declStmt = parent.get<clang::DeclStmt>()) {
            // Check if this DeclStmt is the init part of a ForStmt
            const auto& declStmtParents = m_context.getParents(*declStmt);
            for (const auto& dsParent : declStmtParents) {
                if (const auto* forStmt = dsParent.get<clang::ForStmt>()) {
                    if (forStmt->getInit() == declStmt) {
                        // This variable is in the for-loop init - skip it
                        return true;
                    }
                }
                // Also check for range-based for loops (CXXForRangeStmt)
                if (dsParent.get<clang::CXXForRangeStmt>()) {
                    // This variable is in a range-based for loop - skip it
                    return true;
                }
            }
        }
    }

    clang::QualType type = decl->getType();

    // Check if type is supported
    if (!m_typeEncoder.isSupported(type))
        return true;

    // Skip STL container types - their internals are library-specific
    // and should be handled by the runtime STL encoders, not plugin instrumentation
    if (m_typeEncoder.isStlContainer(type)) {
        return true;
    }

    // For composite types and pointers, ensure type descriptor is emitted
    TypeKind kind = m_typeEncoder.getTypeKind(type);
    if (kind == TypeKind::Struct || kind == TypeKind::Union ||
        kind == TypeKind::Array || kind == TypeKind::Enum ||
        kind == TypeKind::Pointer || kind == TypeKind::Reference) {
        ensureTypeDescriptor(type);
    }

    // Check if the initializer is a new expression - wrap it with capture
    if (const clang::Expr* init = decl->getInit()) {
        // Skip implicit casts to get to the actual expression
        const clang::Expr* initExpr = init->IgnoreImplicit();
        if (const auto* newExpr = llvm::dyn_cast<clang::CXXNewExpr>(initExpr)) {
            clang::QualType allocType = newExpr->getAllocatedType();
            if (m_typeEncoder.isSupported(allocType)) {
                ensureTypeDescriptor(allocType);
                std::string typeRef = m_typeEncoder.getDescriptorRef(allocType);
                std::string typeName = allocType.getAsString();

                bool isArray = newExpr->isArray();

                if (isArray) {
                    const clang::Expr* sizeExpr = newExpr->getArraySize().value_or(nullptr);
                    std::string sizeStr = "1";
                    if (sizeExpr) {
                        clang::SourceRange sizeRange = sizeExpr->getSourceRange();
                        sizeStr = clang::Lexer::getSourceText(
                                      clang::CharSourceRange::getTokenRange(sizeRange),
                                      m_context.getSourceManager(), m_context.getLangOpts())
                                      .str();
                    }
                    std::string prefix = "::inspector::__inspector_capture_new_array<" + typeName + ">(";
                    std::string suffix = ", " + typeRef + ", " + sizeStr + ")";
                    m_rewriter.InsertTextBefore(newExpr->getBeginLoc(), prefix);
                    m_rewriter.InsertTextAfterToken(newExpr->getEndLoc(), suffix);
                } else {
                    std::string prefix = "::inspector::__inspector_capture_new<" + typeName + ">(";
                    std::string suffix = ", " + typeRef + ")";
                    m_rewriter.InsertTextBefore(newExpr->getBeginLoc(), prefix);
                    m_rewriter.InsertTextAfterToken(newExpr->getEndLoc(), suffix);
                }
            }
        }
    }

    std::string call = generateVarInitCall(decl);

    // Insert after the declaration's semicolon
    m_rewriter.InsertTextAfterToken(decl->getEndLoc(), call);

    return true;
}

bool InspectorVisitor::VisitBinaryOperator(clang::BinaryOperator* op) {
    if (!m_helpers.isInMainFile(op->getBeginLoc()))
        return true;

    if (op->getOpcode() != clang::BO_Assign)
        return true;

    // Skip if this expression is the initializer of a VarDecl
    // to avoid conflict with VisitVarDecl's instrumentation
    const auto& parents = m_context.getParents(*op);
    for (const auto& parent : parents) {
        if (parent.get<clang::VarDecl>()) {
            return true;  // Skip - VisitVarDecl will handle this
        }
        // Also check if wrapped in an implicit cast that's a VarDecl initializer
        if (const auto* castExpr = parent.get<clang::ImplicitCastExpr>()) {
            const auto& castParents = m_context.getParents(*castExpr);
            for (const auto& castParent : castParents) {
                if (castParent.get<clang::VarDecl>()) {
                    return true;  // Skip - VisitVarDecl will handle this
                }
            }
        }
    }

    // Get the LHS variable
    clang::Expr* lhs = op->getLHS()->IgnoreParenCasts();
    auto* ref = llvm::dyn_cast<clang::DeclRefExpr>(lhs);
    if (!ref)
        return true;

    auto* var = llvm::dyn_cast<clang::VarDecl>(ref->getDecl());
    if (!var)
        return true;

    clang::QualType type = var->getType();
    if (!m_typeEncoder.isSupported(type))
        return true;

    std::string call = generateVarUpdateCall(var);

    // Wrap in comma operator: (x = expr, __inspector_var_update_...(...))
    m_rewriter.InsertTextBefore(op->getBeginLoc(), "(");
    m_rewriter.InsertTextAfterToken(op->getEndLoc(),
                                    RewriteHelpers::commaWrap(call));

    return true;
}

bool InspectorVisitor::VisitCompoundAssignOperator(
    clang::CompoundAssignOperator* op) {
    if (!m_helpers.isInMainFile(op->getBeginLoc()))
        return true;

    // Skip if this expression is the initializer of a VarDecl
    // to avoid conflict with VisitVarDecl's instrumentation
    const auto& parents = m_context.getParents(*op);
    for (const auto& parent : parents) {
        if (parent.get<clang::VarDecl>()) {
            return true;  // Skip - VisitVarDecl will handle this
        }
        // Also check if wrapped in an implicit cast that's a VarDecl initializer
        if (const auto* castExpr = parent.get<clang::ImplicitCastExpr>()) {
            const auto& castParents = m_context.getParents(*castExpr);
            for (const auto& castParent : castParents) {
                if (castParent.get<clang::VarDecl>()) {
                    return true;  // Skip - VisitVarDecl will handle this
                }
            }
        }
    }

    // Get the LHS variable
    clang::Expr* lhs = op->getLHS()->IgnoreParenCasts();
    auto* ref = llvm::dyn_cast<clang::DeclRefExpr>(lhs);
    if (!ref)
        return true;

    auto* var = llvm::dyn_cast<clang::VarDecl>(ref->getDecl());
    if (!var)
        return true;

    clang::QualType type = var->getType();
    if (!m_typeEncoder.isSupported(type))
        return true;

    std::string call = generateVarUpdateCall(var);

    // Wrap in comma operator
    m_rewriter.InsertTextBefore(op->getBeginLoc(), "(");
    m_rewriter.InsertTextAfterToken(op->getEndLoc(),
                                    RewriteHelpers::commaWrap(call));

    return true;
}

bool InspectorVisitor::VisitUnaryOperator(clang::UnaryOperator* op) {
    if (!m_helpers.isInMainFile(op->getBeginLoc()))
        return true;

    // Handle ++x, --x, x++, x--
    if (!op->isIncrementDecrementOp())
        return true;

    // Skip if this expression is the initializer of a VarDecl
    // to avoid conflict with VisitVarDecl's instrumentation
    const auto& parents = m_context.getParents(*op);
    for (const auto& parent : parents) {
        if (parent.get<clang::VarDecl>()) {
            return true;  // Skip - VisitVarDecl will handle this
        }
        // Also check if wrapped in an implicit cast that's a VarDecl initializer
        if (const auto* castExpr = parent.get<clang::ImplicitCastExpr>()) {
            const auto& castParents = m_context.getParents(*castExpr);
            for (const auto& castParent : castParents) {
                if (castParent.get<clang::VarDecl>()) {
                    return true;  // Skip - VisitVarDecl will handle this
                }
            }
        }
    }

    clang::Expr* sub = op->getSubExpr()->IgnoreParenCasts();
    auto* ref = llvm::dyn_cast<clang::DeclRefExpr>(sub);
    if (!ref)
        return true;

    auto* var = llvm::dyn_cast<clang::VarDecl>(ref->getDecl());
    if (!var)
        return true;

    clang::QualType type = var->getType();
    if (!m_typeEncoder.isSupported(type))
        return true;

    std::string call = generateVarUpdateCall(var);

    // Wrap in comma operator
    m_rewriter.InsertTextBefore(op->getBeginLoc(), "(");
    m_rewriter.InsertTextAfterToken(op->getEndLoc(),
                                    RewriteHelpers::commaWrap(call));

    return true;
}

bool InspectorVisitor::VisitStmt(clang::Stmt* stmt) {
    if (!m_helpers.isInMainFile(stmt->getBeginLoc()))
        return true;

    // Only inject step calls for direct children of CompoundStmt
    if (!hasCompoundStmtParent())
        return true;

    // Skip certain statement types that are handled elsewhere or shouldn't get step calls
    if (llvm::isa<clang::CompoundStmt>(stmt))
        return true;
    if (llvm::isa<clang::DeclStmt>(stmt))
        return true; // Handled by VisitVarDecl
    if (llvm::isa<clang::NullStmt>(stmt))
        return true;
    if (llvm::isa<clang::ReturnStmt>(stmt))
        return true; // Handled by VisitReturnStmt

    // Skip expression statements - they contain the expressions we've already
    // handled with comma operators, and injecting before them causes issues
    // because the step call becomes part of an expression context
    // TODO: Properly handle this by injecting step as separate statement
    // For now, skip step injection on expression statements to avoid breaking code
    if (llvm::isa<clang::Expr>(stmt))
        return true;

    unsigned line = m_helpers.getLineNumber(stmt->getBeginLoc());
    std::string call = "__inspector_step(" + std::to_string(line) + "); ";
    m_rewriter.InsertTextBefore(stmt->getBeginLoc(), call);

    return true;
}

bool InspectorVisitor::VisitIfStmt(clang::IfStmt* stmt) {
    if (!m_helpers.isInMainFile(stmt->getBeginLoc()))
        return true;

    // Ensure then branch is a compound statement
    if (stmt->getThen() && !llvm::isa<clang::CompoundStmt>(stmt->getThen()))
        m_helpers.ensureCompoundBody(stmt->getThen());

    // Ensure else branch is a compound statement (if it exists and isn't
    // another if)
    if (stmt->getElse() && !llvm::isa<clang::IfStmt>(stmt->getElse()) &&
        !llvm::isa<clang::CompoundStmt>(stmt->getElse()))
        m_helpers.ensureCompoundBody(stmt->getElse());

    return true;
}

bool InspectorVisitor::VisitForStmt(clang::ForStmt* stmt) {
    if (!m_helpers.isInMainFile(stmt->getBeginLoc()))
        return true;

    if (stmt->getBody() && !llvm::isa<clang::CompoundStmt>(stmt->getBody()))
        m_helpers.ensureCompoundBody(stmt->getBody());

    return true;
}

bool InspectorVisitor::VisitWhileStmt(clang::WhileStmt* stmt) {
    if (!m_helpers.isInMainFile(stmt->getBeginLoc()))
        return true;

    if (stmt->getBody() && !llvm::isa<clang::CompoundStmt>(stmt->getBody()))
        m_helpers.ensureCompoundBody(stmt->getBody());

    return true;
}

std::string InspectorVisitor::findEnclosingFunctionName(clang::Stmt* stmt) const {
    const auto& parents = m_context.getParents(*stmt);
    for (const auto& parent : parents) {
        if (const auto* fn = parent.get<clang::FunctionDecl>())
            return fn->getNameAsString();
        if (const auto* parentStmt = parent.get<clang::Stmt>())
            return findEnclosingFunctionName(
                const_cast<clang::Stmt*>(parentStmt));
    }
    return "<unknown>";
}

std::string InspectorVisitor::generateVarInitCall(clang::VarDecl* decl) const {
    clang::QualType type = decl->getType();
    std::string varName = decl->getNameAsString();
    std::string suffix = m_typeEncoder.getHookSuffix(type);
    unsigned line = m_helpers.getLineNumber(decl->getLocation());

    std::ostringstream ss;
    TypeKind kind = m_typeEncoder.getTypeKind(type);

    // Use the simple legacy API for int to maintain backward compatibility
    if (kind == TypeKind::Int && type->isSpecificBuiltinType(clang::BuiltinType::Int)) {
        ss << "; __inspector_var_init(\"" << varName << "\", &" << varName << ", " << varName << ")";
    } else if (kind == TypeKind::Struct || kind == TypeKind::Union ||
               kind == TypeKind::Array) {
        // Composite types: pass address, no value expression
        std::string typeRef = m_typeEncoder.getDescriptorRef(type);
        ss << "; __inspector_var_init_" << suffix << "(\"" << varName << "\", &"
           << varName << ", " << typeRef << ", " << line << ")";
    } else {
        // Primitives, pointers, references, enums: pass value
        std::string value = getValueExpr(decl);
        std::string typeRef = m_typeEncoder.getDescriptorRef(type);
        ss << "; __inspector_var_init_" << suffix << "(\"" << varName << "\", &"
           << varName << ", " << typeRef << ", " << value << ", " << line << ")";
    }

    return ss.str();
}

std::string InspectorVisitor::generateVarUpdateCall(clang::VarDecl* decl) const {
    clang::QualType type = decl->getType();
    std::string varName = decl->getNameAsString();
    std::string suffix = m_typeEncoder.getHookSuffix(type);

    std::ostringstream ss;
    TypeKind kind = m_typeEncoder.getTypeKind(type);

    // Use the simple legacy API for int to maintain backward compatibility
    if (kind == TypeKind::Int && type->isSpecificBuiltinType(clang::BuiltinType::Int)) {
        ss << "__inspector_var_update(\"" << varName << "\", &" << varName << ", " << varName << ")";
    } else if (kind == TypeKind::Struct || kind == TypeKind::Union ||
               kind == TypeKind::Array) {
        // Composite types: pass address, no value expression
        std::string typeRef = m_typeEncoder.getDescriptorRef(type);
        ss << "__inspector_var_update_" << suffix << "(\"" << varName << "\", &"
           << varName << ", " << typeRef << ")";
    } else {
        // Primitives, pointers, references, enums: pass value
        std::string value = getValueExpr(decl);
        std::string typeRef = m_typeEncoder.getDescriptorRef(type);
        ss << "__inspector_var_update_" << suffix << "(\"" << varName << "\", &"
           << varName << ", " << typeRef << ", " << value << ")";
    }

    return ss.str();
}

std::string InspectorVisitor::getValueExpr(clang::VarDecl* decl) const {
    clang::QualType type = decl->getType();
    std::string varName = decl->getNameAsString();
    TypeKind kind = m_typeEncoder.getTypeKind(type);

    switch (kind) {
    case TypeKind::Int:
        return "(long long)" + varName;
    case TypeKind::UInt:
        return "(unsigned long long)" + varName;
    case TypeKind::Float:
        return "(double)" + varName;
    case TypeKind::Bool:
        return varName;
    case TypeKind::Char:
        return "(int)" + varName;
    case TypeKind::Pointer:
        return "(const void*)" + varName;
    case TypeKind::Reference:
        return "(const void*)&" + varName;
    case TypeKind::Enum:
        // Cast enum to its underlying type (long long for signed)
        return "(long long)" + varName;
    case TypeKind::Struct:
    case TypeKind::Array:
    case TypeKind::Union:
        // Composite types don't need a value expr - they use address
        return "";
    default:
        break;
    }

    return varName;
}

void InspectorVisitor::ensureTypeDescriptor(clang::QualType type) {
    std::string mangledName = m_typeEncoder.getMangledName(type);

    // Skip if already emitted
    if (m_emittedTypes.count(mangledName))
        return;

    m_emittedTypes.insert(mangledName);

    // Generate descriptor code (this handles dependencies recursively,
    // outputting them before the main type)
    std::string descriptorCode = m_typeEncoder.generateTypeDescriptorCode(type);

    if (!descriptorCode.empty()) {
        // Append descriptors - dependencies come first in the generated code
        m_pendingDescriptors += descriptorCode;
    }
}

clang::SourceLocation InspectorVisitor::getDescriptorInsertionPoint() const {
    return m_insertionPoint;
}

bool InspectorVisitor::VisitCXXNewExpr(clang::CXXNewExpr* expr) {
    if (!m_helpers.isInMainFile(expr->getBeginLoc()))
        return true;

    // Check if this new expression is the direct initializer of a VarDecl.
    // If so, skip here - VisitVarDecl will handle both the capture and the var init.
    const auto& parents = m_context.getParents(*expr);
    for (const auto& parent : parents) {
        if (const auto* varDecl = parent.get<clang::VarDecl>()) {
            // The new expression is the initializer of a variable - skip here
            return true;
        }
        // Also check if wrapped in an implicit cast that's a VarDecl initializer
        if (const auto* castExpr = parent.get<clang::ImplicitCastExpr>()) {
            const auto& castParents = m_context.getParents(*castExpr);
            for (const auto& castParent : castParents) {
                if (castParent.get<clang::VarDecl>()) {
                    return true;
                }
            }
        }
    }

    // Get the allocated type
    clang::QualType allocType = expr->getAllocatedType();
    if (!m_typeEncoder.isSupported(allocType))
        return true;

    // Ensure type descriptor exists
    ensureTypeDescriptor(allocType);
    std::string typeRef = m_typeEncoder.getDescriptorRef(allocType);

    // Get the type name for the template
    std::string typeName = allocType.getAsString();

    // Check if this is array new
    bool isArray = expr->isArray();

    if (isArray) {
        // Get the array size expression
        const clang::Expr* sizeExpr = expr->getArraySize().value_or(nullptr);
        std::string sizeStr = "1";
        if (sizeExpr) {
            clang::SourceRange sizeRange = sizeExpr->getSourceRange();
            sizeStr = clang::Lexer::getSourceText(
                          clang::CharSourceRange::getTokenRange(sizeRange),
                          m_context.getSourceManager(), m_context.getLangOpts())
                          .str();
        }

        // Wrap: ::inspector::__inspector_capture_new_array<T>(new T[n], &type, n)
        std::string prefix = "::inspector::__inspector_capture_new_array<" + typeName + ">(";
        std::string suffix = ", " + typeRef + ", " + sizeStr + ")";

        m_rewriter.InsertTextBefore(expr->getBeginLoc(), prefix);
        m_rewriter.InsertTextAfterToken(expr->getEndLoc(), suffix);
    } else {
        // Wrap: ::inspector::__inspector_capture_new<T>(new T(args), &type)
        std::string prefix = "::inspector::__inspector_capture_new<" + typeName + ">(";
        std::string suffix = ", " + typeRef + ")";

        m_rewriter.InsertTextBefore(expr->getBeginLoc(), prefix);
        m_rewriter.InsertTextAfterToken(expr->getEndLoc(), suffix);
    }

    return true;
}

bool InspectorVisitor::VisitCXXDeleteExpr(clang::CXXDeleteExpr* expr) {
    if (!m_helpers.isInMainFile(expr->getBeginLoc()))
        return true;

    // Get the pointer expression being deleted
    const clang::Expr* argExpr = expr->getArgument();
    if (!argExpr)
        return true;

    // Get the source text of the argument
    clang::SourceRange argRange = argExpr->getSourceRange();
    std::string argStr = clang::Lexer::getSourceText(
                             clang::CharSourceRange::getTokenRange(argRange),
                             m_context.getSourceManager(), m_context.getLangOpts())
                             .str();

    // Check if it's delete[] or delete
    bool isArray = expr->isArrayForm();

    // Get the type of what's being deleted for the template
    clang::QualType argType = argExpr->getType();
    std::string typeName = "void"; // Default fallback
    if (const auto* ptrType = argType->getAs<clang::PointerType>()) {
        typeName = ptrType->getPointeeType().getAsString();
    }

    // Wrap: (::inspector::__inspector_pre_delete<T>(p), delete p)
    // or:   (::inspector::__inspector_pre_delete_array<T>(p), delete[] p)
    std::string hookName = isArray ? "__inspector_pre_delete_array" : "__inspector_pre_delete";
    std::string prefix = "(::inspector::" + hookName + "<" + typeName + ">(" + argStr + "), ";
    std::string suffix = ")";

    m_rewriter.InsertTextBefore(expr->getBeginLoc(), prefix);
    m_rewriter.InsertTextAfterToken(expr->getEndLoc(), suffix);

    return true;
}

bool InspectorVisitor::VisitCXXThrowExpr(clang::CXXThrowExpr* expr) {
    if (!m_helpers.isInMainFile(expr->getBeginLoc()))
        return true;

    unsigned line = m_helpers.getLineNumber(expr->getBeginLoc());
    std::string funcName = m_currentFunction.empty() ? "<unknown>" : m_currentFunction;

    // Insert throw event before the throw expression
    std::string call = "(__inspector_throw(\"" + funcName + "\", " + std::to_string(line) + "), ";
    m_rewriter.InsertTextBefore(expr->getBeginLoc(), call);
    m_rewriter.InsertTextAfterToken(expr->getEndLoc(), ")");

    return true;
}

bool InspectorVisitor::VisitCXXCatchStmt(clang::CXXCatchStmt* stmt) {
    if (!m_helpers.isInMainFile(stmt->getBeginLoc()))
        return true;

    unsigned line = m_helpers.getLineNumber(stmt->getBeginLoc());
    std::string funcName = m_currentFunction.empty() ? "<unknown>" : m_currentFunction;

    // Get exception type name
    std::string typeName = "...";  // Default for catch(...)
    if (stmt->getExceptionDecl()) {
        typeName = stmt->getExceptionDecl()->getType().getAsString();
    }

    // Insert catch event at the beginning of the catch body
    if (auto* body = stmt->getHandlerBlock()) {
        if (auto* compound = llvm::dyn_cast<clang::CompoundStmt>(body)) {
            std::string call = "__inspector_catch(\"" + funcName + "\", \"" +
                               typeName + "\", " + std::to_string(line) + "); ";
            m_rewriter.InsertTextAfterToken(compound->getLBracLoc(), "\n        " + call);
        }
    }

    return true;
}

} // namespace inspector
