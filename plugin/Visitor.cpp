//! @file Visitor.cpp
//! @brief Implementation of the See++ AST visitor.

#include "Visitor.h"

#include "clang/AST/ParentMapContext.h"
#include "clang/Lex/Lexer.h"

#include <sstream>

namespace see {

SeeVisitor::SeeVisitor(clang::Rewriter& rewriter, clang::ASTContext& context)
    : m_helpers(rewriter, context), m_typeEncoder(context), m_context(context),
      m_rewriter(rewriter) {}

bool SeeVisitor::TraverseStmt(clang::Stmt* stmt) {
    if (!stmt)
        return true;

    m_parentStack.push_back(stmt);
    bool result = clang::RecursiveASTVisitor<SeeVisitor>::TraverseStmt(stmt);
    m_parentStack.pop_back();
    return result;
}

bool SeeVisitor::hasCompoundStmtParent() const {
    // The current statement is at the back, its parent is second-to-last
    if (m_parentStack.size() < 2)
        return false;
    return llvm::isa<clang::CompoundStmt>(
        m_parentStack[m_parentStack.size() - 2]);
}

bool SeeVisitor::VisitFunctionDecl(clang::FunctionDecl* decl) {
    if (!decl->hasBody() || !m_helpers.isInMainFile(decl->getLocation()))
        return true;

    clang::Stmt* body = decl->getBody();
    auto* compound = llvm::dyn_cast<clang::CompoundStmt>(body);
    if (!compound)
        return true;

    std::string funcName = decl->getNameAsString();
    m_currentFunction = funcName;

    // Inject __see_enter after opening brace
    unsigned enterLine = m_helpers.getLineNumber(compound->getLBracLoc());
    std::string enterCall =
        "__see_enter(\"" + funcName + "\", " + std::to_string(enterLine) + "); ";
    m_rewriter.InsertTextAfterToken(compound->getLBracLoc(),
                                    "\n    " + enterCall);

    // Inject __see_leave before closing brace
    unsigned leaveLine = m_helpers.getLineNumber(compound->getRBracLoc());
    std::string leaveCall =
        "    __see_leave(\"" + funcName + "\", " + std::to_string(leaveLine) +
        ");\n";
    m_rewriter.InsertTextBefore(compound->getRBracLoc(), leaveCall);

    return true;
}

bool SeeVisitor::VisitReturnStmt(clang::ReturnStmt* stmt) {
    if (!m_helpers.isInMainFile(stmt->getBeginLoc()))
        return true;

    std::string funcName = findEnclosingFunctionName(stmt);
    unsigned line = m_helpers.getLineNumber(stmt->getBeginLoc());
    std::string call =
        "__see_leave(\"" + funcName + "\", " + std::to_string(line) + "); ";
    m_rewriter.InsertTextBefore(stmt->getBeginLoc(), call);

    return true;
}

bool SeeVisitor::VisitVarDecl(clang::VarDecl* decl) {
    if (!m_helpers.isInMainFile(decl->getLocation()))
        return true;

    // Only instrument local variables
    if (!decl->isLocalVarDecl())
        return true;

    clang::QualType type = decl->getType();

    // Check if type is supported
    if (!m_typeEncoder.isSupported(type))
        return true;

    std::string call = generateVarInitCall(decl);

    // Insert after the declaration's semicolon
    m_rewriter.InsertTextAfterToken(decl->getEndLoc(), call);

    return true;
}

bool SeeVisitor::VisitBinaryOperator(clang::BinaryOperator* op) {
    if (!m_helpers.isInMainFile(op->getBeginLoc()))
        return true;

    if (op->getOpcode() != clang::BO_Assign)
        return true;

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

    // Wrap in comma operator: (x = expr, __see_var_update_...(...))
    m_rewriter.InsertTextBefore(op->getBeginLoc(), "(");
    m_rewriter.InsertTextAfterToken(op->getEndLoc(),
                                    RewriteHelpers::commaWrap(call));

    return true;
}

bool SeeVisitor::VisitCompoundAssignOperator(
    clang::CompoundAssignOperator* op) {
    if (!m_helpers.isInMainFile(op->getBeginLoc()))
        return true;

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

bool SeeVisitor::VisitUnaryOperator(clang::UnaryOperator* op) {
    if (!m_helpers.isInMainFile(op->getBeginLoc()))
        return true;

    // Handle ++x, --x, x++, x--
    if (!op->isIncrementDecrementOp())
        return true;

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

bool SeeVisitor::VisitStmt(clang::Stmt* stmt) {
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
    std::string call = "__see_step(" + std::to_string(line) + "); ";
    m_rewriter.InsertTextBefore(stmt->getBeginLoc(), call);

    return true;
}

bool SeeVisitor::VisitIfStmt(clang::IfStmt* stmt) {
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

bool SeeVisitor::VisitForStmt(clang::ForStmt* stmt) {
    if (!m_helpers.isInMainFile(stmt->getBeginLoc()))
        return true;

    if (stmt->getBody() && !llvm::isa<clang::CompoundStmt>(stmt->getBody()))
        m_helpers.ensureCompoundBody(stmt->getBody());

    return true;
}

bool SeeVisitor::VisitWhileStmt(clang::WhileStmt* stmt) {
    if (!m_helpers.isInMainFile(stmt->getBeginLoc()))
        return true;

    if (stmt->getBody() && !llvm::isa<clang::CompoundStmt>(stmt->getBody()))
        m_helpers.ensureCompoundBody(stmt->getBody());

    return true;
}

std::string SeeVisitor::findEnclosingFunctionName(clang::Stmt* stmt) const {
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

std::string SeeVisitor::generateVarInitCall(clang::VarDecl* decl) const {
    clang::QualType type = decl->getType();
    std::string varName = decl->getNameAsString();
    std::string suffix = m_typeEncoder.getHookSuffix(type);
    std::string value = getValueExpr(decl);
    unsigned line = m_helpers.getLineNumber(decl->getLocation());

    // For now, use simplified API without type descriptors
    // InsertTextAfterToken puts text after the last token of the decl (before ;)
    // So we need to add our own semicolon to terminate the declaration,
    // then our call (the original ; becomes a null statement which is fine)
    std::ostringstream ss;

    TypeKind kind = m_typeEncoder.getTypeKind(type);

    // Use the simple legacy API for int to maintain backward compatibility
    if (kind == TypeKind::Int && type->isSpecificBuiltinType(clang::BuiltinType::Int)) {
        ss << "; __see_var_init(\"" << varName << "\", &" << varName << ", " << varName << ")";
    } else {
        ss << "; __see_var_init_" << suffix << "(\"" << varName << "\", &"
           << varName << ", nullptr, " << value << ", " << line << ")";
    }

    return ss.str();
}

std::string SeeVisitor::generateVarUpdateCall(clang::VarDecl* decl) const {
    clang::QualType type = decl->getType();
    std::string varName = decl->getNameAsString();
    std::string suffix = m_typeEncoder.getHookSuffix(type);
    std::string value = getValueExpr(decl);

    std::ostringstream ss;

    TypeKind kind = m_typeEncoder.getTypeKind(type);

    // Use the simple legacy API for int to maintain backward compatibility
    if (kind == TypeKind::Int && type->isSpecificBuiltinType(clang::BuiltinType::Int)) {
        ss << "__see_var_update(\"" << varName << "\", &" << varName << ", " << varName << ")";
    } else {
        ss << "__see_var_update_" << suffix << "(\"" << varName << "\", &"
           << varName << ", nullptr, " << value << ")";
    }

    return ss.str();
}

std::string SeeVisitor::getValueExpr(clang::VarDecl* decl) const {
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
    default:
        break;
    }

    return varName;
}

} // namespace see
