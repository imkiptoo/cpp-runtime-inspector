//! @file Visitor.h
//! @brief RecursiveASTVisitor for C++ Runtime Inspector instrumentation.

#pragma once

#include "Diagnostics.h"
#include "RewriteHelpers.h"
#include "TypeEncoder.h"

#include "clang/AST/RecursiveASTVisitor.h"

#include <string>
#include <unordered_set>
#include <vector>

namespace inspector {

//! AST visitor that walks the translation unit and rewrites source code
//! to inject instrumentation calls.
class InspectorVisitor : public clang::RecursiveASTVisitor<InspectorVisitor> {
public:
    InspectorVisitor(clang::Rewriter& rewriter, clang::ASTContext& context);

    //! Finalize the visitor - flush pending type descriptors.
    void finalize();

    //! Enable base class traversal for statement children.
    bool shouldTraversePostOrder() const { return false; }

    //! Custom traversal to track parent stack.
    bool TraverseStmt(clang::Stmt* stmt);

    //! Visit function definitions to inject enter/leave calls.
    bool VisitFunctionDecl(clang::FunctionDecl* decl);

    //! Visit return statements to inject leave calls.
    bool VisitReturnStmt(clang::ReturnStmt* stmt);

    //! Visit variable declarations to inject init calls.
    bool VisitVarDecl(clang::VarDecl* decl);

    //! Visit binary operators to inject update calls for assignments.
    bool VisitBinaryOperator(clang::BinaryOperator* op);

    //! Visit compound assignment operators (+=, -=, etc.).
    bool VisitCompoundAssignOperator(clang::CompoundAssignOperator* op);

    //! Visit unary operators for ++/-- tracking.
    bool VisitUnaryOperator(clang::UnaryOperator* op);

    //! Visit statements to inject step calls.
    bool VisitStmt(clang::Stmt* stmt);

    //! Visit if statements to ensure compound bodies.
    bool VisitIfStmt(clang::IfStmt* stmt);

    //! Visit for statements to ensure compound bodies.
    bool VisitForStmt(clang::ForStmt* stmt);

    //! Visit while statements to ensure compound bodies.
    bool VisitWhileStmt(clang::WhileStmt* stmt);

    //! Visit new expressions to capture heap allocations (Tier 3).
    bool VisitCXXNewExpr(clang::CXXNewExpr* expr);

    //! Visit delete expressions to track deallocations (Tier 3).
    bool VisitCXXDeleteExpr(clang::CXXDeleteExpr* expr);

private:
    //! Check if current statement's parent is a CompoundStmt.
    bool hasCompoundStmtParent() const;

    //! Find the enclosing function name for a statement.
    std::string findEnclosingFunctionName(clang::Stmt* stmt) const;

    //! Generate the appropriate hook call for a variable declaration.
    std::string generateVarInitCall(clang::VarDecl* decl) const;

    //! Generate the appropriate hook call for a variable update.
    std::string generateVarUpdateCall(clang::VarDecl* decl) const;

    //! Get the value expression for passing to a hook.
    std::string getValueExpr(clang::VarDecl* decl) const;

    //! Ensure type descriptor is emitted for a type (for Tier 2 composite types).
    void ensureTypeDescriptor(clang::QualType type);

    //! Get the location to insert type descriptors (before main or first function).
    clang::SourceLocation getDescriptorInsertionPoint() const;

    //! Track which types we've generated descriptors for.
    mutable std::unordered_set<std::string> m_emittedTypes;

    //! Accumulated type descriptor code to be inserted.
    mutable std::string m_pendingDescriptors;

    //! Track if we've found an insertion point.
    mutable bool m_hasInsertionPoint = false;
    mutable clang::SourceLocation m_insertionPoint;

    //! Stack of parent statements for context tracking.
    std::vector<clang::Stmt*> m_parentStack;

    //! Current function name (set during function visitation).
    std::string m_currentFunction;

    //! Helpers
    RewriteHelpers m_helpers;
    TypeEncoder m_typeEncoder;
    clang::ASTContext& m_context;
    clang::Rewriter& m_rewriter;
};

} // namespace inspector
