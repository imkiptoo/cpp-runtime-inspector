//! @file SeePlugin.cpp
//! @brief Clang plugin registration for See++ instrumentation.
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

//! ASTConsumer that runs the See++ visitor on the translation unit.
class SeeConsumer : public clang::ASTConsumer {
public:
    void HandleTranslationUnit(clang::ASTContext& context) override {
        clang::Rewriter rewriter;
        rewriter.setSourceMgr(context.getSourceManager(),
                              context.getLangOpts());

        see::SeeVisitor visitor(rewriter, context);
        visitor.TraverseDecl(context.getTranslationUnitDecl());

        // Write instrumented output
        clang::SourceManager& sm = context.getSourceManager();
        clang::FileID mainFid = sm.getMainFileID();
        const llvm::RewriteBuffer* buf = rewriter.getRewriteBufferFor(mainFid);

        clang::OptionalFileEntryRef fe = sm.getFileEntryRefForID(mainFid);
        if (!fe) {
            llvm::errs() << "see-instrument: could not resolve main file\n";
            return;
        }
        std::string outPath = fe->getName().str() + ".instrumented.cpp";

        std::error_code ec;
        llvm::raw_fd_ostream out(outPath, ec);
        if (ec) {
            llvm::errs() << "see-instrument: could not open " << outPath
                         << ": " << ec.message() << "\n";
            return;
        }

        // Inject runtime header
        out << "#include \"see_runtime.h\"\n";

        if (buf) {
            out << std::string(buf->begin(), buf->end());
        } else {
            llvm::StringRef origBuf = sm.getBufferData(mainFid);
            out << origBuf.str();
        }
        out.flush();

        llvm::errs() << "see-instrument: wrote " << outPath << "\n";
    }
};

//! Plugin action that instantiates the consumer.
class SeePluginAction : public clang::PluginASTAction {
public:
    std::unique_ptr<clang::ASTConsumer>
    CreateASTConsumer(clang::CompilerInstance&, llvm::StringRef) override {
        return std::make_unique<SeeConsumer>();
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

static clang::FrontendPluginRegistry::Add<SeePluginAction>
    g_register("see-instrument",
               "Instrument C++ source for See++-style tracing");
