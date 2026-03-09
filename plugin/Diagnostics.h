//! @file Diagnostics.h
//! @brief Warning and error emission utilities for the See++ plugin.

#pragma once

#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/SourceLocation.h"

namespace see {

//! Diagnostic helper for emitting plugin-specific warnings and notes.
class Diagnostics {
public:
    explicit Diagnostics(clang::DiagnosticsEngine& diags);

    //! Emit a warning about an unsupported type.
    void warnUnsupportedType(clang::SourceLocation loc,
                             llvm::StringRef typeName);

    //! Emit a warning about skipped instrumentation.
    void warnSkippedInstrumentation(clang::SourceLocation loc,
                                    llvm::StringRef reason);

    //! Emit a note with additional context.
    void note(clang::SourceLocation loc, llvm::StringRef message);

private:
    clang::DiagnosticsEngine& m_diags;
    unsigned m_warnUnsupportedTypeID;
    unsigned m_warnSkippedID;
    unsigned m_noteID;
};

} // namespace see
