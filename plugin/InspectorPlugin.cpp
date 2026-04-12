//! @file InspectorPlugin.cpp
//! @brief Clang plugin registration for C++ Runtime Inspector instrumentation.
//!
//! This file only contains plugin registration. The actual instrumentation
//! logic is in Visitor.cpp, with helpers in TypeEncoder.cpp, RewriteHelpers.cpp,
//! and Diagnostics.cpp.

#include "Visitor.h"

#include "clang/AST/ASTConsumer.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/Rewrite/Core/Rewriter.h"

#include <memory>

namespace {

//! ASTConsumer that runs the C++ Runtime Inspector visitor on the translation unit.
class InspectorConsumer : public clang::ASTConsumer {
public:
    void HandleTranslationUnit(clang::ASTContext& context) override {
        clang::Rewriter rewriter;
        rewriter.setSourceMgr(context.getSourceManager(),
                              context.getLangOpts());

        inspector::InspectorVisitor visitor(rewriter, context);
        visitor.TraverseDecl(context.getTranslationUnitDecl());
        visitor.finalize();

        // Write instrumented output
        clang::SourceManager& sm = context.getSourceManager();
        clang::FileID mainFid = sm.getMainFileID();
        const llvm::RewriteBuffer* buf = rewriter.getRewriteBufferFor(mainFid);

        clang::OptionalFileEntryRef fe = sm.getFileEntryRefForID(mainFid);
        if (!fe) {
            llvm::errs() << "inspector-instrument: could not resolve main file\n";
            return;
        }
        std::string outPath = fe->getName().str() + ".instrumented.cpp";

        std::error_code ec;
        llvm::raw_fd_ostream out(outPath, ec);
        if (ec) {
            llvm::errs() << "inspector-instrument: could not open " << outPath
                         << ": " << ec.message() << "\n";
            return;
        }

        // Inject runtime header
        out << "#include \"inspector_runtime.h\"\n";

        if (buf) {
            out << std::string(buf->begin(), buf->end());
        } else {
            llvm::StringRef origBuf = sm.getBufferData(mainFid);
            out << origBuf.str();
        }
        out.flush();

        llvm::errs() << "inspector-instrument: wrote " << outPath << "\n";
    }
};

//! Plugin action that instantiates the consumer.
class InspectorPluginAction : public clang::PluginASTAction {
public:
    std::unique_ptr<clang::ASTConsumer>
    CreateASTConsumer(clang::CompilerInstance&, llvm::StringRef) override {
        return std::make_unique<InspectorConsumer>();
    }

    bool ParseArgs(const clang::CompilerInstance&,
                   const std::vector<std::string>&) override {
        return true;
    }

    PluginASTAction::ActionType getActionType() override {
        return PluginASTAction::AddBeforeMainAction;
    }
};

} // namespace

static clang::FrontendPluginRegistry::Add<InspectorPluginAction>
    g_register("inspector-instrument",
               "Instrument C++ source for C++ Runtime Inspector tracing");

