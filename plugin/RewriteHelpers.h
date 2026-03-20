//! @file RewriteHelpers.h
//! @brief Source rewriting utilities for the See++ plugin.

#pragma once

#include "clang/AST/ASTContext.h"
#include "clang/AST/Stmt.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include <string>

namespace inspector {

//! Helper class for source code rewriting operations.
class RewriteHelpers {
public:
    RewriteHelpers(clang::Rewriter& rewriter, clang::ASTContext& context);

    //! Insert text after the specified location's token.
    void insertAfterToken(clang::SourceLocation loc, llvm::StringRef text);

    //! Insert text before the specified location.
    void insertBefore(clang::SourceLocation loc, llvm::StringRef text);

    //! Get the location just past the end of a token.
    clang::SourceLocation getLocAfterToken(clang::SourceLocation loc);

    //! Build a function call expression string.
    static std::string buildCall(llvm::StringRef funcName,
                                 llvm::ArrayRef<std::string> args);

    //! Build a string literal for injection.
    static std::string stringLiteral(llvm::StringRef s);

    //! Wrap an expression with parentheses and comma operator.
    //! Returns the closing part: ", __see_...(...))"
    static std::string commaWrap(llvm::StringRef hookCall);

    //! Get source line number for a location.
    unsigned getLineNumber(clang::SourceLocation loc) const;

    //! Check if location is in main file (not system headers).
    bool isInMainFile(clang::SourceLocation loc) const;

    //! Ensure a statement body is a CompoundStmt, rewriting if necessary.
    //! For `if (x) stmt;` → `if (x) { stmt; }`
    //! Returns true if rewriting was performed.
    bool ensureCompoundBody(clang::Stmt* body);

private:
    clang::Rewriter& m_rewriter;
    clang::ASTContext& m_context;
};

} // namespace inspector
