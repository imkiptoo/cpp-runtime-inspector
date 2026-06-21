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

    //! Skip traversal of constexpr/consteval functions: their bodies cannot
    //! contain side-effecting hook calls.
    bool TraverseFunctionDecl(clang::FunctionDecl* decl);
    bool TraverseCXXMethodDecl(clang::CXXMethodDecl* decl);
    bool TraverseCXXConstructorDecl(clang::CXXConstructorDecl* decl);
    bool TraverseCXXDestructorDecl(clang::CXXDestructorDecl* decl);
    bool TraverseCXXConversionDecl(clang::CXXConversionDecl* decl);

    //! Visit function definitions to inject enter/leave calls.
    bool VisitFunctionDecl(clang::FunctionDecl* decl);

    //! Visit return statements to inject leave calls.
    bool VisitReturnStmt(clang::ReturnStmt* stmt);

    //! Visit variable declarations to inject init calls.
    bool VisitVarDecl(clang::VarDecl* decl);

    //! Visit declaration statements to handle multi-variable declarations.
    bool VisitDeclStmt(clang::DeclStmt* stmt);

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

    //! Visit range-based for loops to instrument the loop variable.
    bool VisitCXXForRangeStmt(clang::CXXForRangeStmt* stmt);

    //! Visit while statements to ensure compound bodies.
    bool VisitWhileStmt(clang::WhileStmt* stmt);

    //! Visit new expressions to capture heap allocations (Tier 3).
    bool VisitCXXNewExpr(clang::CXXNewExpr* expr);

    //! Visit delete expressions to track deallocations (Tier 3).
    bool VisitCXXDeleteExpr(clang::CXXDeleteExpr* expr);

    //! Visit throw expressions to track exceptions (Tier 5).
    bool VisitCXXThrowExpr(clang::CXXThrowExpr* expr);

    //! Visit catch statements to track exception handling (Tier 5).
    bool VisitCXXCatchStmt(clang::CXXCatchStmt* stmt);

    //! Visit constructor declarations to instrument with lifecycle tracking.
    //! Detects default, copy, and move constructors.
    bool VisitCXXConstructorDecl(clang::CXXConstructorDecl* decl);

    //! Visit destructor declarations to instrument with lifecycle tracking.
    bool VisitCXXDestructorDecl(clang::CXXDestructorDecl* decl);

    //! Visit expressions with cleanups to track temporary destructions.
    //! These wrap full-expressions that have temporaries needing cleanup.
    bool VisitExprWithCleanups(clang::ExprWithCleanups* expr);

private:
    //! Collect CXXBindTemporaryExpr nodes from a subtree.
    //! Returns the types of temporaries that will be destroyed.
    void collectTemporaries(clang::Stmt* stmt, std::vector<std::pair<std::string, unsigned>>& temps);
    //! Walk an LHS expression to determine if it's a write through `this->`.
    //! Returns true if the LHS chain ultimately roots in a CXXThisExpr.
    bool isWriteThroughThis(clang::Expr* lhs) const;

    //! Wrap an expression that writes through `this->` so a step fires
    //! after the write. Live re-encoding then snapshots the receiver in
    //! the caller's frame.
    void wrapThisWriteWithStep(clang::Expr* expr, unsigned line);

    //! Wrap a local-variable write as `(expr, updateCall, __inspector_step(line))`
    //! so the write records the new value AND emits a step on that line.
    void wrapLocalWriteWithStep(clang::Expr* op, const std::string& updateCall,
                                unsigned line);

    //! Check if current statement's parent is a CompoundStmt.
    bool hasCompoundStmtParent() const;

    //! Find the enclosing function name for a statement.
    std::string findEnclosingFunctionName(clang::Stmt* stmt) const;

    //! Generate the appropriate hook call for a variable declaration.
    std::string generateVarInitCall(clang::VarDecl* decl) const;

    //! Like generateVarInitCall but for a structured-binding BindingDecl.
    std::string generateInitCallForBinding(clang::BindingDecl* binding) const;

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

    //! Track DeclStmts with multiple VarDecls that are handled by VisitDeclStmt.
    std::unordered_set<const clang::DeclStmt*> m_multiVarDeclStmts;

    //! Track function bodies that have already been instrumented.
    std::unordered_set<const clang::FunctionDecl*> m_instrumentedFunctions;

    //! Track new expressions that have already been wrapped (by pointer).
    std::unordered_set<const clang::CXXNewExpr*> m_processedNewExprs;

    //! Track new expressions by source location (in case of different AST nodes).
    std::unordered_set<unsigned> m_processedNewLocations;

    //! Current function name (set during function visitation).
    std::string m_currentFunction;

    //! Helpers
    RewriteHelpers m_helpers;
    TypeEncoder m_typeEncoder;
    clang::ASTContext& m_context;
    clang::Rewriter& m_rewriter;
};

} // namespace inspector
