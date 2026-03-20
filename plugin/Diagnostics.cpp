//! @file Diagnostics.cpp
//! @brief Implementation of diagnostic emission utilities.

#include "Diagnostics.h"

namespace inspector {

Diagnostics::Diagnostics(clang::DiagnosticsEngine& diags)
    : m_diags(diags)
{
    // Register custom diagnostics
    m_warnUnsupportedTypeID = m_diags.getCustomDiagID(
        clang::DiagnosticsEngine::Warning,
        "inspector-instrument: unsupported type '%0', skipping instrumentation");

    m_warnSkippedID = m_diags.getCustomDiagID(
        clang::DiagnosticsEngine::Warning,
        "inspector-instrument: skipped instrumentation: %0");

    m_noteID = m_diags.getCustomDiagID(
        clang::DiagnosticsEngine::Note, "inspector-instrument: %0");
}

void Diagnostics::warnUnsupportedType(clang::SourceLocation loc,
                                      llvm::StringRef typeName) {
    m_diags.Report(loc, m_warnUnsupportedTypeID) << typeName;
}

void Diagnostics::warnSkippedInstrumentation(clang::SourceLocation loc,
                                             llvm::StringRef reason) {
    m_diags.Report(loc, m_warnSkippedID) << reason;
}

void Diagnostics::note(clang::SourceLocation loc, llvm::StringRef message) {
    m_diags.Report(loc, m_noteID) << message;
}

} // namespace inspector
