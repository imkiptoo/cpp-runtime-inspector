//! @file RewriteHelpers.cpp
//! @brief Implementation of source rewriting utilities.

#include "RewriteHelpers.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Lex/Lexer.h"

namespace inspector {

RewriteHelpers::RewriteHelpers(clang::Rewriter& rewriter,
                               clang::ASTContext& context)
    : m_rewriter(rewriter), m_context(context) {}

void RewriteHelpers::insertAfterToken(clang::SourceLocation loc,
                                      llvm::StringRef text) {
    m_rewriter.InsertTextAfterToken(loc, text);
}

void RewriteHelpers::insertBefore(clang::SourceLocation loc,
                                  llvm::StringRef text) {
    m_rewriter.InsertTextBefore(loc, text);
}

clang::SourceLocation
RewriteHelpers::getLocAfterToken(clang::SourceLocation loc) {
    return clang::Lexer::getLocForEndOfToken(
        loc, 0, m_context.getSourceManager(), m_context.getLangOpts());
}

std::string RewriteHelpers::buildCall(llvm::StringRef funcName,
                                      llvm::ArrayRef<std::string> args) {
    std::string result;
    result += funcName;
    result += "(";
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0)
            result += ", ";
        result += args[i];
    }
    result += ")";
    return result;
}

std::string RewriteHelpers::stringLiteral(llvm::StringRef s) {
    std::string result = "\"";
    for (char c : s) {
        switch (c) {
        case '"':
            result += "\\\"";
            break;
        case '\\':
            result += "\\\\";
            break;
        case '\n':
            result += "\\n";
            break;
        case '\r':
            result += "\\r";
            break;
        case '\t':
            result += "\\t";
            break;
        default:
            result += c;
        }
    }
    result += "\"";
    return result;
}

std::string RewriteHelpers::commaWrap(llvm::StringRef hookCall) {
    std::string result = ", ";
    result += hookCall;
    result += ")";
    return result;
}

unsigned RewriteHelpers::getLineNumber(clang::SourceLocation loc) const {
    return m_context.getSourceManager().getSpellingLineNumber(loc);
}

bool RewriteHelpers::isInMainFile(clang::SourceLocation loc) const {
    if (!loc.isValid())
        return false;
    return m_context.getSourceManager().isInMainFile(loc);
}

bool RewriteHelpers::ensureCompoundBody(clang::Stmt* body) {
    if (!body || llvm::isa<clang::CompoundStmt>(body))
        return false;

    // Insert braces around the statement
    m_rewriter.InsertTextBefore(body->getBeginLoc(), "{ ");

    clang::SourceLocation endLoc = clang::Lexer::getLocForEndOfToken(
        body->getEndLoc(), 0, m_context.getSourceManager(),
        m_context.getLangOpts());
    m_rewriter.InsertTextAfterToken(body->getEndLoc(), " }");

    return true;
}

} // namespace inspector
