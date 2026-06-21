//! @file Visitor.cpp
//! @brief Implementation of the C++ Runtime Inspector AST visitor.

#include "Visitor.h"

#include "clang/AST/ParentMapContext.h"
#include "clang/Basic/SourceManager.h"
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

bool InspectorVisitor::TraverseFunctionDecl(clang::FunctionDecl* decl) {
    if (decl && decl->isConstexpr())
        return true;
    return clang::RecursiveASTVisitor<InspectorVisitor>::TraverseFunctionDecl(decl);
}

bool InspectorVisitor::TraverseCXXMethodDecl(clang::CXXMethodDecl* decl) {
    if (decl && decl->isConstexpr())
        return true;
    return clang::RecursiveASTVisitor<InspectorVisitor>::TraverseCXXMethodDecl(decl);
}

bool InspectorVisitor::TraverseCXXConstructorDecl(clang::CXXConstructorDecl* decl) {
    if (decl && decl->isConstexpr())
        return true;
    return clang::RecursiveASTVisitor<InspectorVisitor>::TraverseCXXConstructorDecl(decl);
}

bool InspectorVisitor::TraverseCXXDestructorDecl(clang::CXXDestructorDecl* decl) {
    if (decl && decl->isConstexpr())
        return true;
    return clang::RecursiveASTVisitor<InspectorVisitor>::TraverseCXXDestructorDecl(decl);
}

bool InspectorVisitor::TraverseCXXConversionDecl(clang::CXXConversionDecl* decl) {
    if (decl && decl->isConstexpr())
        return true;
    return clang::RecursiveASTVisitor<InspectorVisitor>::TraverseCXXConversionDecl(decl);
}

bool InspectorVisitor::VisitFunctionDecl(clang::FunctionDecl* decl) {
    if (!decl->hasBody() || !m_helpers.isInMainFile(decl->getLocation()))
        return true;

    // Skip constructors and destructors - they have their own visitor methods
    if (llvm::isa<clang::CXXConstructorDecl>(decl) ||
        llvm::isa<clang::CXXDestructorDecl>(decl))
        return true;

    clang::Stmt* body = decl->getBody();
    auto* compound = llvm::dyn_cast<clang::CompoundStmt>(body);
    if (!compound)
        return true;

    // Skip if already instrumented (handles forward declarations + definitions)
    // Use canonical declaration so forward decl and definition map to same key
    const clang::FunctionDecl* canonical = decl->getCanonicalDecl();
    if (m_instrumentedFunctions.count(canonical))
        return true;
    m_instrumentedFunctions.insert(canonical);

    // Use qualified name so namespace and class scopes are visible:
    // math::square, Counter::bump. Free functions in the global namespace
    // are unchanged.
    std::string funcName = decl->getQualifiedNameAsString();
    m_currentFunction = funcName;

    // Check if this is a copy or move assignment operator
    int lifecycleKind = 0;  // None
    if (auto* method = llvm::dyn_cast<clang::CXXMethodDecl>(decl)) {
        if (method->isCopyAssignmentOperator()) {
            lifecycleKind = 4;  // CopyAssign
        } else if (method->isMoveAssignmentOperator()) {
            lifecycleKind = 5;  // MoveAssign
        }
    }

    // Pick an insertion point for type descriptors. Descriptors must come
    // *before* any code that references them; that includes member
    // functions defined inline inside a class.
    //
    // For free functions we use the function's own start location. For
    // methods we walk up to the outermost enclosing class — and if that
    // class is the body of a class template, up once more to the
    // ClassTemplateDecl, so descriptors don't land between
    // `template <...>` and `struct Foo`. We keep the earliest seen.
    {
        clang::SourceLocation candidate;
        if (decl->isCXXClassMember()) {
            const clang::DeclContext* ctx = decl->getDeclContext();
            const clang::CXXRecordDecl* outer = nullptr;
            while (ctx) {
                if (auto* rec = llvm::dyn_cast<clang::CXXRecordDecl>(ctx)) {
                    outer = rec;
                }
                ctx = ctx->getParent();
            }
            if (outer) {
                if (auto* tmpl = outer->getDescribedClassTemplate()) {
                    candidate = tmpl->getBeginLoc();
                } else {
                    candidate = outer->getBeginLoc();
                }
            }
        } else {
            candidate = decl->getBeginLoc();
        }

        if (candidate.isValid()) {
            const auto& sm = m_context.getSourceManager();
            if (!m_hasInsertionPoint ||
                sm.isBeforeInTranslationUnit(candidate, m_insertionPoint)) {
                m_insertionPoint = candidate;
                m_hasInsertionPoint = true;
            }
        }
    }

    // Inject __inspector_enter or __inspector_enter_lifecycle after opening brace
    unsigned enterLine = m_helpers.getLineNumber(compound->getLBracLoc());
    std::string enterCall;
    if (lifecycleKind != 0) {
        enterCall = "__inspector_enter_lifecycle(\"" + funcName + "\", " +
                    std::to_string(enterLine) + ", " + std::to_string(lifecycleKind) + "); ";
    } else {
        enterCall = "__inspector_enter(\"" + funcName + "\", " + std::to_string(enterLine) + "); ";
    }

    // Generate parameter init calls
    std::string paramCalls;
    for (const clang::ParmVarDecl* param : decl->parameters()) {
        clang::QualType type = param->getType();
        std::string paramName = param->getNameAsString();
        if (paramName.empty()) continue;  // Skip unnamed parameters

        // Skip dependent types (template parameters)
        if (type->isDependentType())
            continue;

        if (!m_typeEncoder.isSupported(type))
            continue;

        // Ensure type descriptor for composite types
        TypeKind kind = m_typeEncoder.getTypeKind(type);
        if (kind == TypeKind::Struct || kind == TypeKind::Union ||
            kind == TypeKind::Array || kind == TypeKind::Enum ||
            kind == TypeKind::Pointer || kind == TypeKind::Reference) {
            ensureTypeDescriptor(type);
        }

        // Generate param init call (reuse var_init format)
        std::string suffix = m_typeEncoder.getHookSuffix(type);
        unsigned paramLine = m_helpers.getLineNumber(param->getLocation());

        if (kind == TypeKind::Struct || kind == TypeKind::Union || kind == TypeKind::Array) {
            std::string typeRef = m_typeEncoder.getDescriptorRef(type);
            paramCalls += " __inspector_var_init_" + suffix + "(\"" + paramName + "\", &" +
                          paramName + ", " + typeRef + ", " + std::to_string(paramLine) + ");";
        } else {
            std::string typeRef = m_typeEncoder.getDescriptorRef(type);
            std::string value;
            switch (kind) {
            case TypeKind::Int:    value = std::string("(long long)") + paramName; break;
            case TypeKind::UInt:   value = std::string("(unsigned long long)") + paramName; break;
            case TypeKind::Float:  value = std::string("(double)") + paramName; break;
            case TypeKind::Bool:   value = paramName; break;
            case TypeKind::Char:   value = std::string("(int)") + paramName; break;
            case TypeKind::Pointer:
            case TypeKind::Reference:
                value = std::string("(const void*)") + (kind == TypeKind::Reference ? "&" : "") + paramName;
                break;
            case TypeKind::Enum:   value = std::string("(long long)") + paramName; break;
            default:               value = "0"; break;
            }
            paramCalls += " __inspector_var_init_" + suffix + "(\"" + paramName + "\", &" +
                          paramName + ", " + typeRef + ", " + value + ", " + std::to_string(paramLine) + ");";
        }
    }

    m_rewriter.InsertTextAfterToken(compound->getLBracLoc(),
                                    "\n    " + enterCall + paramCalls);

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

    // Capture return value if present
    std::string retCapture;
    bool needsReplacement = false;  // True if we need to replace the entire return stmt
    std::string replacementStmt;    // The replacement code

    if (const clang::Expr* retVal = stmt->getRetValue()) {
        clang::QualType retType = retVal->getType();

        // Check if return expression contains a function call
        const clang::Expr* retValNoParens = retVal->IgnoreParenImpCasts();
        bool isCallExpr = llvm::isa<clang::CallExpr>(retValNoParens) ||
                          llvm::isa<clang::CXXMemberCallExpr>(retValNoParens) ||
                          llvm::isa<clang::CXXOperatorCallExpr>(retValNoParens);

        if (m_typeEncoder.isSupported(retType) && !retType->isVoidType()) {
            TypeKind kind = m_typeEncoder.getTypeKind(retType);

            // Get source text for return expression
            clang::SourceRange range = retVal->getSourceRange();
            std::string retExpr = clang::Lexer::getSourceText(
                clang::CharSourceRange::getTokenRange(range),
                m_context.getSourceManager(), m_context.getLangOpts()).str();

            // Build the capture call based on type
            std::string captureCall;
            if (kind == TypeKind::Int && retType->isSpecificBuiltinType(clang::BuiltinType::Int)) {
                captureCall = "__inspector_return_int";
            } else if (kind == TypeKind::Int) {
                captureCall = "__inspector_return_int";
            } else if (kind == TypeKind::UInt) {
                captureCall = "__inspector_return_uint";
            } else if (kind == TypeKind::Float) {
                captureCall = "__inspector_return_float";
            } else if (kind == TypeKind::Bool) {
                captureCall = "__inspector_return_bool";
            } else if (kind == TypeKind::Char) {
                captureCall = "__inspector_return_char";
            } else if (kind == TypeKind::Pointer) {
                captureCall = "__inspector_return_ptr";
            }

            if (!captureCall.empty()) {
                if (isCallExpr) {
                    // For function calls, use a temp variable to avoid double execution
                    // { auto __ret = expr; capture(__ret); leave(); return __ret; }
                    needsReplacement = true;
                    std::string leave = "__inspector_leave(\"" + funcName + "\", " +
                                        std::to_string(line) + ")";
                    replacementStmt = "{ auto __inspector_ret = " + retExpr + "; " +
                                      captureCall + "(__inspector_ret); " +
                                      leave + "; return __inspector_ret; }";
                } else {
                    // For simple expressions, capture directly (no double execution)
                    if (kind == TypeKind::Int && !retType->isSpecificBuiltinType(clang::BuiltinType::Int)) {
                        retCapture = captureCall + "((long long)(" + retExpr + ")); ";
                    } else if (kind == TypeKind::UInt) {
                        retCapture = captureCall + "((unsigned long long)(" + retExpr + ")); ";
                    } else if (kind == TypeKind::Float) {
                        retCapture = captureCall + "((double)(" + retExpr + ")); ";
                    } else if (kind == TypeKind::Pointer) {
                        retCapture = captureCall + "((const void*)(" + retExpr + ")); ";
                    } else {
                        retCapture = captureCall + "(" + retExpr + "); ";
                    }
                }
            }
            // Skip complex types (struct, array) for now - would need temporary storage
        }
    }

    if (needsReplacement) {
        // Replace the entire return statement
        clang::SourceRange stmtRange = stmt->getSourceRange();
        // Need to include the semicolon
        clang::SourceLocation endLoc = clang::Lexer::findLocationAfterToken(
            stmtRange.getEnd(), clang::tok::semi, m_context.getSourceManager(),
            m_context.getLangOpts(), /*SkipTrailingWhitespaceAndNewLine=*/false);
        if (endLoc.isValid()) {
            m_rewriter.ReplaceText(clang::SourceRange(stmtRange.getBegin(),
                                   endLoc.getLocWithOffset(-1)), replacementStmt);
        } else {
            // Fallback: just replace the statement without the semicolon
            m_rewriter.ReplaceText(stmtRange, replacementStmt);
        }
    } else {
        std::string call = retCapture +
            "__inspector_leave(\"" + funcName + "\", " + std::to_string(line) + "); ";
        // InsertAfter=true so the leave call sits *inside* any synthetic braces
        // added by ensureCompoundBody for if/else/while/for bodies that consist
        // of a single return statement. With InsertTextBefore the leave would
        // land outside the braces and produce invalid syntax.
        m_rewriter.InsertText(stmt->getBeginLoc(), call, /*InsertAfter=*/true);
    }

    return true;
}

bool InspectorVisitor::VisitVarDecl(clang::VarDecl* decl) {
    if (!m_helpers.isInMainFile(decl->getLocation()))
        return true;

    // Handle global/constexpr variables at file scope
    if (!decl->isLocalVarDecl()) {
        // Only instrument variables with initializers (so we can capture their value)
        if (!decl->hasInit())
            return true;

        // Only support primitive types for globals
        clang::QualType type = decl->getType();
        if (!m_typeEncoder.isSupported(type))
            return true;

        TypeKind kind = m_typeEncoder.getTypeKind(type);
        // Only primitives and enums for now
        if (kind != TypeKind::Int && kind != TypeKind::UInt &&
            kind != TypeKind::Float && kind != TypeKind::Bool &&
            kind != TypeKind::Char && kind != TypeKind::Enum)
            return true;

        std::string name = decl->getNameAsString();
        if (name.empty())
            return true;

        // Ensure type descriptor is emitted
        ensureTypeDescriptor(type);
        std::string typeRef = m_typeEncoder.getDescriptorRef(type);

        // Build the global registration call
        std::string hookCall;
        // Use qualified name for static members (Counter::total), simple name otherwise
        std::string valueExpr = decl->getQualifiedNameAsString();

        if (kind == TypeKind::Int) {
            hookCall = "__inspector_global_int(\"" + name + "\", " + typeRef +
                       ", (long long)(" + valueExpr + "))";
        } else if (kind == TypeKind::UInt) {
            hookCall = "__inspector_global_uint(\"" + name + "\", " + typeRef +
                       ", (unsigned long long)(" + valueExpr + "))";
        } else if (kind == TypeKind::Float) {
            hookCall = "__inspector_global_float(\"" + name + "\", " + typeRef +
                       ", (double)(" + valueExpr + "))";
        } else if (kind == TypeKind::Bool) {
            hookCall = "__inspector_global_bool(\"" + name + "\", " + typeRef + ", " + valueExpr + ")";
        } else if (kind == TypeKind::Char) {
            hookCall = "__inspector_global_char(\"" + name + "\", " + typeRef +
                       ", (int)(" + valueExpr + "))";
        } else if (kind == TypeKind::Enum) {
            hookCall = "__inspector_global_enum(\"" + name + "\", " + typeRef +
                       ", (long long)(" + valueExpr + "))";
        }

        if (!hookCall.empty()) {
            // Find the semicolon after the declaration
            clang::SourceLocation semiLoc = clang::Lexer::findLocationAfterToken(
                decl->getEndLoc(), clang::tok::semi, m_context.getSourceManager(),
                m_context.getLangOpts(), /*SkipTrailingWhitespaceAndNewLine=*/false);

            if (semiLoc.isValid()) {
                std::string staticInit;

                // For enum globals, we need an extern declaration for the type descriptor
                // since it will be defined later in the file
                if (kind == TypeKind::Enum) {
                    std::string mangledName = m_typeEncoder.getMangledName(type);
                    staticInit = "\nextern const inspector::TypeDescriptor __inspector_type_" +
                                 mangledName + ";";
                }

                // Generate a static initializer to register the global
                staticInit += "\nstatic int __inspector_init_" + name +
                              " = (" + hookCall + ", 0);";
                m_rewriter.InsertText(semiLoc, staticInit);
            }
        }

        return true;
    }

    // Skip exception catch clause declarations
    if (decl->isExceptionVariable())
        return true;

    // Structured bindings: emit one __inspector_var_init per BindingDecl.
    // The DecompositionDecl itself has no name, so we inject after the
    // statement's last token (the initializer's end). Each binding is a
    // valid lvalue that supports unary &.
    if (auto* decomp = llvm::dyn_cast<clang::DecompositionDecl>(decl)) {
        std::string injected;
        for (clang::BindingDecl* binding : decomp->bindings()) {
            clang::QualType bt = binding->getType();
            if (!m_typeEncoder.isSupported(bt))
                continue;
            if (m_typeEncoder.isStlContainer(bt))
                continue;
            TypeKind k = m_typeEncoder.getTypeKind(bt);
            if (k == TypeKind::Struct || k == TypeKind::Union ||
                k == TypeKind::Array || k == TypeKind::Enum ||
                k == TypeKind::Pointer || k == TypeKind::Reference) {
                ensureTypeDescriptor(bt);
            }
            injected += generateInitCallForBinding(binding);
        }
        if (!injected.empty())
            m_rewriter.InsertTextAfterToken(decl->getEndLoc(), injected);
        return true;
    }

    // Check parent DeclStmt for:
    // 1. Multi-variable declarations (e.g., "int a, b;") - already handled by VisitDeclStmt
    // 2. For-loop init expressions - cannot be instrumented
    const auto& parents = m_context.getParents(*decl);
    for (const auto& parent : parents) {
        if (const auto* declStmt = parent.get<clang::DeclStmt>()) {
            // If this DeclStmt was already processed by VisitDeclStmt, skip
            if (m_multiVarDeclStmts.count(declStmt) > 0) {
                return true;
            }

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

    // STL containers reach var_init_struct, which forwards to the runtime
    // STL encoder. We still skip plugin-side field walks (handled by the
    // type-descriptor block below not running for these types) but we now
    // emit the init call so the local variable shows up in the trace.

    // For composite types and pointers, ensure type descriptor is emitted
    TypeKind kind = m_typeEncoder.getTypeKind(type);
    if (kind == TypeKind::Struct || kind == TypeKind::Union ||
        kind == TypeKind::Array || kind == TypeKind::Enum ||
        kind == TypeKind::Pointer || kind == TypeKind::Reference) {
        ensureTypeDescriptor(type);
    }

    // Note: new expressions in initializers are handled by VisitCXXNewExpr,
    // so we don't wrap them here to avoid double-wrapping.

    std::string call = generateVarInitCall(decl);

    // Find the semicolon after the declaration to insert after it.
    // This is important when the initializer contains a new expression that
    // gets wrapped - we need var_init to come AFTER the capture_new wrapper.
    clang::SourceLocation semiLoc = clang::Lexer::findLocationAfterToken(
        decl->getEndLoc(), clang::tok::semi, m_context.getSourceManager(),
        m_context.getLangOpts(), /*SkipTrailingWhitespaceAndNewLine=*/false);

    if (semiLoc.isValid()) {
        // Insert after the semicolon (the call already starts with ";")
        // Remove the leading ";" from call since we're inserting after the existing semicolon
        // and add a trailing ";" since we're now a separate statement
        std::string callWithoutSemi = call;
        if (!callWithoutSemi.empty() && callWithoutSemi[0] == ';') {
            callWithoutSemi = callWithoutSemi.substr(1);
        }
        m_rewriter.InsertText(semiLoc, callWithoutSemi + ";");
    } else {
        // Fallback to old behavior if semicolon not found
        m_rewriter.InsertTextAfterToken(decl->getEndLoc(), call);
    }

    return true;
}

bool InspectorVisitor::VisitDeclStmt(clang::DeclStmt* stmt) {
    if (!m_helpers.isInMainFile(stmt->getBeginLoc()))
        return true;

    // Count VarDecls in this DeclStmt to detect multi-variable declarations
    int varDeclCount = 0;
    for (const auto* d : stmt->decls()) {
        if (llvm::isa<clang::VarDecl>(d))
            varDeclCount++;
    }

    // Only handle multi-variable declarations (e.g., "int a, b;")
    // Single variable declarations are handled by VisitVarDecl
    if (varDeclCount <= 1)
        return true;

    // Mark this DeclStmt so VisitVarDecl knows to skip its VarDecls
    m_multiVarDeclStmts.insert(stmt);

    // Check if this DeclStmt is in a for-loop init - skip if so
    const auto& parents = m_context.getParents(*stmt);
    for (const auto& parent : parents) {
        if (const auto* forStmt = parent.get<clang::ForStmt>()) {
            if (forStmt->getInit() == stmt)
                return true;  // Skip for-loop init variables
        }
        if (parent.get<clang::CXXForRangeStmt>())
            return true;  // Skip range-based for loop variables
    }

    // Collect all VarDecls and generate instrumentation for each
    std::string allCalls;
    for (const auto* d : stmt->decls()) {
        const auto* varDecl = llvm::dyn_cast<clang::VarDecl>(d);
        if (!varDecl)
            continue;

        // Skip non-local variables
        if (!varDecl->isLocalVarDecl())
            continue;

        clang::QualType type = varDecl->getType();

        // Check if type is supported
        if (!m_typeEncoder.isSupported(type))
            continue;

        // For composite types and pointers, ensure type descriptor is emitted
        TypeKind kind = m_typeEncoder.getTypeKind(type);
        if (kind == TypeKind::Struct || kind == TypeKind::Union ||
            kind == TypeKind::Array || kind == TypeKind::Enum ||
            kind == TypeKind::Pointer || kind == TypeKind::Reference) {
            ensureTypeDescriptor(type);
        }

        // Generate the init call for this variable
        allCalls += generateVarInitCall(const_cast<clang::VarDecl*>(varDecl));
    }

    // Insert all calls after the entire DeclStmt (after the semicolon)
    // generateVarInitCall prepends "; " to each call, which works when inserting
    // after a VarDecl. But for DeclStmt, we're already after the semicolon,
    // so we need to trim the leading "; " and add a trailing semicolon.
    if (!allCalls.empty()) {
        // Remove leading "; " (2 chars) and add trailing ";"
        if (allCalls.size() >= 2 && allCalls[0] == ';' && allCalls[1] == ' ') {
            allCalls = " " + allCalls.substr(2) + ";";
        }
        m_rewriter.InsertTextAfterToken(stmt->getEndLoc(), allCalls);
    }

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

    // Get the LHS variable. Two shapes are supported:
    //   x = expr              (DeclRefExpr LHS)
    //   x.member = expr       (MemberExpr whose base is a DeclRefExpr to a
    //                          local var; we re-encode the whole base var)
    clang::Expr* lhs = op->getLHS()->IgnoreParenCasts();
    clang::VarDecl* var = nullptr;
    if (auto* ref = llvm::dyn_cast<clang::DeclRefExpr>(lhs)) {
        var = llvm::dyn_cast<clang::VarDecl>(ref->getDecl());
    } else if (auto* member = llvm::dyn_cast<clang::MemberExpr>(lhs)) {
        // Walk through nested member exprs to find a base DeclRefExpr. We
        // only recapture writes through `.` (or nested `.` chains), not
        // through `->` — pointer-target writes mutate the heap, not the
        // pointer itself, and conflict with the new-capture wrapping in
        // VisitCXXNewExpr.
        bool throughPointer = false;
        clang::Expr* base = member;
        while (auto* m = llvm::dyn_cast<clang::MemberExpr>(base)) {
            if (m->isArrow()) {
                throughPointer = true;
                break;
            }
            base = m->getBase()->IgnoreParenCasts();
        }
        if (!throughPointer) {
            if (auto* baseRef = llvm::dyn_cast<clang::DeclRefExpr>(base))
                var = llvm::dyn_cast<clang::VarDecl>(baseRef->getDecl());
        }
    }

    // If the LHS is a write through `this->...`, we can't update a specific
    // local — `this` points into the caller's frame. Instead, fire a step
    // post-write so live re-encoding in emitStep snapshots every frame
    // (including the caller's, where the receiver actually lives).
    if (!var && isWriteThroughThis(lhs)) {
        unsigned line = m_helpers.getLineNumber(op->getBeginLoc());
        wrapThisWriteWithStep(op, line);
        return true;
    }

    if (!var)
        return true;

    clang::QualType type = var->getType();
    if (!m_typeEncoder.isSupported(type))
        return true;

    // Skip non-local VarDecl writes that we can't safely emit a descriptor
    // for at the LHS use-site. This covers:
    //   - global/static struct members: descriptor only emitted for locals
    //   - reference parameters whose member is written: the descriptor is
    //     for the *referent* (also unemitted), and the runtime would track
    //     the parameter as a fake local
    //   - struct-typed parameters by value: same — we don't track params
    // Drop in a step instead so live re-encoding snapshots the caller's
    // frame, which is where the mutated object actually lives.
    if (!var->isLocalVarDecl()) {
        TypeKind kind = m_typeEncoder.getTypeKind(type);
        if (kind == TypeKind::Struct || kind == TypeKind::Union ||
            kind == TypeKind::Array || kind == TypeKind::Reference) {
            unsigned line = m_helpers.getLineNumber(op->getBeginLoc());
            wrapThisWriteWithStep(op, line);
            return true;
        }
    }

    std::string call = generateVarUpdateCall(var);
    unsigned line = m_helpers.getLineNumber(op->getBeginLoc());

    // Wrap as (x = expr, __inspector_var_update_...(...), __inspector_step(line))
    wrapLocalWriteWithStep(op, call, line);

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

    // Handle compound assign through `this->` (e.g., this->total += n).
    if (!ref && isWriteThroughThis(lhs)) {
        unsigned line = m_helpers.getLineNumber(op->getBeginLoc());
        wrapThisWriteWithStep(op, line);
        return true;
    }

    if (!ref)
        return true;

    auto* var = llvm::dyn_cast<clang::VarDecl>(ref->getDecl());
    if (!var)
        return true;

    clang::QualType type = var->getType();
    if (!m_typeEncoder.isSupported(type))
        return true;

    // Same composite-non-local guard as in VisitBinaryOperator.
    if (!var->isLocalVarDecl()) {
        TypeKind kind = m_typeEncoder.getTypeKind(type);
        if (kind == TypeKind::Struct || kind == TypeKind::Union ||
            kind == TypeKind::Array || kind == TypeKind::Reference) {
            unsigned line = m_helpers.getLineNumber(op->getBeginLoc());
            wrapThisWriteWithStep(op, line);
            return true;
        }
    }

    std::string call = generateVarUpdateCall(var);
    unsigned line = m_helpers.getLineNumber(op->getBeginLoc());

    // Wrap as (expr, __inspector_var_update_...(...), __inspector_step(line))
    wrapLocalWriteWithStep(op, call, line);

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

    // Handle ++this->x / --this->x.
    if (!ref && isWriteThroughThis(sub)) {
        unsigned line = m_helpers.getLineNumber(op->getBeginLoc());
        wrapThisWriteWithStep(op, line);
        return true;
    }

    if (!ref)
        return true;

    auto* var = llvm::dyn_cast<clang::VarDecl>(ref->getDecl());
    if (!var)
        return true;

    clang::QualType type = var->getType();
    if (!m_typeEncoder.isSupported(type))
        return true;

    // Same composite-non-local guard as in VisitBinaryOperator.
    if (!var->isLocalVarDecl()) {
        TypeKind kind = m_typeEncoder.getTypeKind(type);
        if (kind == TypeKind::Struct || kind == TypeKind::Union ||
            kind == TypeKind::Array || kind == TypeKind::Reference) {
            unsigned line = m_helpers.getLineNumber(op->getBeginLoc());
            wrapThisWriteWithStep(op, line);
            return true;
        }
    }

    std::string call = generateVarUpdateCall(var);
    unsigned line = m_helpers.getLineNumber(op->getBeginLoc());

    // Wrap as (expr, __inspector_var_update_...(...), __inspector_step(line))
    wrapLocalWriteWithStep(op, call, line);

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
        return true; // Handled by VisitVarDecl/VisitDeclStmt (var_init emits the step)
    if (llvm::isa<clang::NullStmt>(stmt))
        return true;
    if (llvm::isa<clang::ReturnStmt>(stmt))
        return true; // Handled by VisitReturnStmt

    unsigned line = m_helpers.getLineNumber(stmt->getBeginLoc());

    // Expression statements (e.g. `std::cout << ...;`, `foo();`): step *after*
    // the expression so the snapshot captures its effects — stdout produced by
    // the statement, heap/global mutations from a call, etc. Use the same comma
    // wrap as this-writes: `(expr, __inspector_step(line))`.
    if (auto* expr = llvm::dyn_cast<clang::Expr>(stmt)) {
        const clang::Expr* inner = expr->IgnoreImplicit()->IgnoreParens();

        // Variable-write expressions are stepped by VisitBinaryOperator /
        // VisitCompoundAssignOperator / VisitUnaryOperator (they wrap the write
        // with a var_update + step). Stepping here too would double-count.
        if (const auto* bin = llvm::dyn_cast<clang::BinaryOperator>(inner)) {
            if (bin->isAssignmentOp())
                return true;
        }
        if (const auto* un = llvm::dyn_cast<clang::UnaryOperator>(inner)) {
            if (un->isIncrementDecrementOp())
                return true;
        }
        // `throw` is handled by VisitCXXThrowExpr; a step after it is unreachable.
        if (llvm::isa<clang::CXXThrowExpr>(inner))
            return true;

        m_rewriter.InsertText(expr->getBeginLoc(), "(", /*InsertAfter=*/true);
        m_rewriter.InsertTextAfterToken(
            expr->getEndLoc(),
            ", __inspector_step(" + std::to_string(line) + "))");
        return true;
    }

    // Non-expression statements (if / for / while / switch ...): step before so
    // the line is highlighted as control flow reaches it.
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

bool InspectorVisitor::VisitCXXForRangeStmt(clang::CXXForRangeStmt* stmt) {
    if (!m_helpers.isInMainFile(stmt->getBeginLoc()))
        return true;

    clang::Stmt* body = stmt->getBody();
    if (!body)
        return true;

    bool wasCompound = llvm::isa<clang::CompoundStmt>(body);
    if (!wasCompound)
        m_helpers.ensureCompoundBody(body);

    clang::VarDecl* loopVar = stmt->getLoopVariable();
    if (!loopVar || !m_typeEncoder.isSupported(loopVar->getType()))
        return true;
    if (m_typeEncoder.isStlContainer(loopVar->getType()))
        return true;

    clang::QualType type = loopVar->getType();
    TypeKind kind = m_typeEncoder.getTypeKind(type);
    if (kind == TypeKind::Struct || kind == TypeKind::Union ||
        kind == TypeKind::Array || kind == TypeKind::Enum ||
        kind == TypeKind::Pointer || kind == TypeKind::Reference) {
        ensureTypeDescriptor(type);
    }

    // generateVarInitCall produces "; __inspector_var_init(...)". We're
    // inserting at the start of the body, so we want a leading space and
    // a trailing ';'.
    std::string call = generateVarInitCall(loopVar);
    if (!call.empty() && call.front() == ';')
        call.erase(0, 1); // drop leading ';'
    std::string injected = " " + call + "; ";

    if (wasCompound) {
        auto* compound = llvm::cast<clang::CompoundStmt>(body);
        m_rewriter.InsertTextAfterToken(compound->getLBracLoc(), injected);
    } else {
        m_rewriter.InsertText(body->getBeginLoc(), injected,
                              /*InsertAfter=*/true);
    }

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
            return fn->getQualifiedNameAsString();
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

    // Composite types: pass address, no value expression. (int and other
    // primitives go through the full _<suffix> API below so the step carries
    // the correct line number — the legacy __inspector_var_init records line 0.)
    if (kind == TypeKind::Struct || kind == TypeKind::Union ||
        kind == TypeKind::Array) {
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

std::string InspectorVisitor::generateInitCallForBinding(
    clang::BindingDecl* binding) const {
    clang::QualType type = binding->getType();
    std::string varName = binding->getNameAsString();
    std::string suffix = m_typeEncoder.getHookSuffix(type);
    unsigned line = m_helpers.getLineNumber(binding->getLocation());
    TypeKind kind = m_typeEncoder.getTypeKind(type);

    std::ostringstream ss;
    if (kind == TypeKind::Int && type->isSpecificBuiltinType(clang::BuiltinType::Int)) {
        ss << "; __inspector_var_init(\"" << varName << "\", &" << varName
           << ", " << varName << ")";
    } else if (kind == TypeKind::Struct || kind == TypeKind::Union ||
               kind == TypeKind::Array) {
        std::string typeRef = m_typeEncoder.getDescriptorRef(type);
        ss << "; __inspector_var_init_" << suffix << "(\"" << varName
           << "\", &" << varName << ", " << typeRef << ", " << line << ")";
    } else {
        // Reuse the value-expr scheme. Bindings have no VarDecl, so we
        // inline the cast logic here.
        std::string value;
        switch (kind) {
        case TypeKind::Int:    value = "(long long)" + varName; break;
        case TypeKind::UInt:   value = "(unsigned long long)" + varName; break;
        case TypeKind::Float:  value = "(double)" + varName; break;
        case TypeKind::Bool:   value = varName; break;
        case TypeKind::Char:   value = "(int)" + varName; break;
        case TypeKind::Pointer:   value = "(const void*)" + varName; break;
        case TypeKind::Reference: value = "(const void*)&" + varName; break;
        case TypeKind::Enum:   value = "(long long)" + varName; break;
        default:               value = varName; break;
        }
        std::string typeRef = m_typeEncoder.getDescriptorRef(type);
        ss << "; __inspector_var_init_" << suffix << "(\"" << varName
           << "\", &" << varName << ", " << typeRef << ", " << value
           << ", " << line << ")";
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

    // Guard against processing the same new expression twice (by pointer)
    if (m_processedNewExprs.count(expr)) {
        llvm::errs() << "inspector-instrument: SKIPPING duplicate new expr (by pointer)\n";
        return true;
    }

    // Also guard by source location to catch different AST nodes for same source
    clang::SourceLocation loc = expr->getBeginLoc();
    unsigned locOffset = m_context.getSourceManager().getFileOffset(loc);
    if (m_processedNewLocations.count(locOffset)) {
        llvm::errs() << "inspector-instrument: SKIPPING duplicate new expr (by location " << locOffset << ")\n";
        return true;
    }

    llvm::errs() << "inspector-instrument: wrapping new expr at offset " << locOffset << "\n";
    m_processedNewExprs.insert(expr);
    m_processedNewLocations.insert(locOffset);

    // All new expressions are handled here - VisitVarDecl only handles the
    // variable initialization tracking, not the new expression wrapping.

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

        unsigned beginOffset = m_context.getSourceManager().getFileOffset(expr->getBeginLoc());
        unsigned endOffset = m_context.getSourceManager().getFileOffset(expr->getEndLoc());
        llvm::errs() << "inspector-instrument: inserting at begin=" << beginOffset
                     << " end=" << endOffset << " type=" << typeName << "\n";

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

    // Insert throw event before the throw expression. Use InsertAfter=true
    // so the wrapper sits inside any synthetic braces ensureCompoundBody
    // adds for if/else/while/for bodies that consist of a single throw.
    std::string call = "(__inspector_throw(\"" + funcName + "\", " + std::to_string(line) + "), ";
    m_rewriter.InsertText(expr->getBeginLoc(), call, /*InsertAfter=*/true);
    m_rewriter.InsertTextAfterToken(expr->getEndLoc(), ")");

    return true;
}

bool InspectorVisitor::isWriteThroughThis(clang::Expr* lhs) const {
    if (!lhs)
        return false;
    auto* member = llvm::dyn_cast<clang::MemberExpr>(lhs);
    if (!member)
        return false;
    // Walk down the member chain, then check the innermost base.
    clang::Expr* base = member;
    while (auto* m = llvm::dyn_cast<clang::MemberExpr>(base)) {
        base = m->getBase()->IgnoreParenImpCasts();
    }
    return llvm::isa<clang::CXXThisExpr>(base);
}

void InspectorVisitor::wrapThisWriteWithStep(clang::Expr* expr, unsigned line) {
    // Wrap as (expr, __inspector_step(line)). The step fires post-write so
    // emitStep's live re-encoder snapshots the receiver's caller frame.
    std::string suffix =
        ", __inspector_step(" + std::to_string(line) + "))";
    m_rewriter.InsertText(expr->getBeginLoc(), "(", /*InsertAfter=*/true);
    m_rewriter.InsertTextAfterToken(expr->getEndLoc(), suffix);
}

void InspectorVisitor::wrapLocalWriteWithStep(clang::Expr* op,
                                              const std::string& updateCall,
                                              unsigned line) {
    // Wrap as (expr, var_update(...), __inspector_step(line)): record the new
    // value, then fire a step so the write shows up as its own trace step.
    m_rewriter.InsertText(op->getBeginLoc(), "(", /*InsertAfter=*/true);
    m_rewriter.InsertTextAfterToken(
        op->getEndLoc(),
        ", " + updateCall + ", __inspector_step(" + std::to_string(line) + "))");
}

bool InspectorVisitor::VisitCXXConstructorDecl(clang::CXXConstructorDecl* decl) {
    if (!decl->hasBody() || !m_helpers.isInMainFile(decl->getLocation()))
        return true;

    // Skip if already instrumented (VisitFunctionDecl may also visit constructors)
    const clang::FunctionDecl* canonical = decl->getCanonicalDecl();
    if (m_instrumentedFunctions.count(canonical))
        return true;
    m_instrumentedFunctions.insert(canonical);

    clang::Stmt* body = decl->getBody();
    auto* compound = llvm::dyn_cast<clang::CompoundStmt>(body);
    if (!compound)
        return true;

    std::string funcName = decl->getQualifiedNameAsString();
    m_currentFunction = funcName;

    // Determine the lifecycle kind
    int lifecycleKind = 0;  // None
    if (decl->isDefaultConstructor()) {
        lifecycleKind = 1;  // DefaultCtor
    } else if (decl->isCopyConstructor()) {
        lifecycleKind = 2;  // CopyCtor
    } else if (decl->isMoveConstructor()) {
        lifecycleKind = 3;  // MoveCtor
    }

    // Set insertion point for type descriptors (same logic as VisitFunctionDecl)
    {
        clang::SourceLocation candidate;
        const clang::DeclContext* ctx = decl->getDeclContext();
        const clang::CXXRecordDecl* outer = nullptr;
        while (ctx) {
            if (auto* rec = llvm::dyn_cast<clang::CXXRecordDecl>(ctx)) {
                outer = rec;
            }
            ctx = ctx->getParent();
        }
        if (outer) {
            if (auto* tmpl = outer->getDescribedClassTemplate()) {
                candidate = tmpl->getBeginLoc();
            } else {
                candidate = outer->getBeginLoc();
            }
        }

        if (candidate.isValid()) {
            const auto& sm = m_context.getSourceManager();
            if (!m_hasInsertionPoint ||
                sm.isBeforeInTranslationUnit(candidate, m_insertionPoint)) {
                m_insertionPoint = candidate;
                m_hasInsertionPoint = true;
            }
        }
    }

    // Inject __inspector_enter_lifecycle after opening brace
    unsigned enterLine = m_helpers.getLineNumber(compound->getLBracLoc());
    std::string enterCall =
        "__inspector_enter_lifecycle(\"" + funcName + "\", " +
        std::to_string(enterLine) + ", " + std::to_string(lifecycleKind) + "); ";
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

bool InspectorVisitor::VisitCXXDestructorDecl(clang::CXXDestructorDecl* decl) {
    if (!decl->hasBody() || !m_helpers.isInMainFile(decl->getLocation()))
        return true;

    // Skip if already instrumented (VisitFunctionDecl may also visit destructors)
    const clang::FunctionDecl* canonical = decl->getCanonicalDecl();
    if (m_instrumentedFunctions.count(canonical))
        return true;
    m_instrumentedFunctions.insert(canonical);

    clang::Stmt* body = decl->getBody();
    auto* compound = llvm::dyn_cast<clang::CompoundStmt>(body);
    if (!compound)
        return true;

    std::string funcName = decl->getQualifiedNameAsString();
    m_currentFunction = funcName;

    // Lifecycle kind 6 = Dtor
    int lifecycleKind = 6;

    // Set insertion point for type descriptors
    {
        clang::SourceLocation candidate;
        const clang::DeclContext* ctx = decl->getDeclContext();
        const clang::CXXRecordDecl* outer = nullptr;
        while (ctx) {
            if (auto* rec = llvm::dyn_cast<clang::CXXRecordDecl>(ctx)) {
                outer = rec;
            }
            ctx = ctx->getParent();
        }
        if (outer) {
            if (auto* tmpl = outer->getDescribedClassTemplate()) {
                candidate = tmpl->getBeginLoc();
            } else {
                candidate = outer->getBeginLoc();
            }
        }

        if (candidate.isValid()) {
            const auto& sm = m_context.getSourceManager();
            if (!m_hasInsertionPoint ||
                sm.isBeforeInTranslationUnit(candidate, m_insertionPoint)) {
                m_insertionPoint = candidate;
                m_hasInsertionPoint = true;
            }
        }
    }

    // Inject __inspector_enter_lifecycle after opening brace
    unsigned enterLine = m_helpers.getLineNumber(compound->getLBracLoc());
    std::string enterCall =
        "__inspector_enter_lifecycle(\"" + funcName + "\", " +
        std::to_string(enterLine) + ", " + std::to_string(lifecycleKind) + "); ";
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

void InspectorVisitor::collectTemporaries(clang::Stmt* stmt,
    std::vector<std::pair<std::string, unsigned>>& temps) {
    if (!stmt)
        return;

    // If this is a CXXBindTemporaryExpr, record the temporary's type
    if (auto* bindTemp = llvm::dyn_cast<clang::CXXBindTemporaryExpr>(stmt)) {
        clang::QualType type = bindTemp->getType();
        std::string typeName = type.getAsString();

        // Clean up the type name - remove references and const qualifiers for display
        clang::QualType cleanType = type.getNonReferenceType();
        cleanType.removeLocalConst();
        typeName = cleanType.getAsString();

        // Get the line number for this temporary
        unsigned line = m_helpers.getLineNumber(bindTemp->getBeginLoc());

        temps.push_back({typeName, line});
    }

    // Recursively check children
    for (clang::Stmt* child : stmt->children()) {
        collectTemporaries(child, temps);
    }
}

bool InspectorVisitor::VisitExprWithCleanups(clang::ExprWithCleanups* expr) {
    if (!m_helpers.isInMainFile(expr->getBeginLoc()))
        return true;

    // Note: getNumObjects() may return 0 even when there are temporaries to clean up
    // because Clang may handle cleanup differently. We check for CXXBindTemporaryExpr
    // children instead.

    // Skip if this is part of a VarDecl initializer - those are handled by VisitVarDecl
    // and wrapping them would break the variable declaration
    const auto& parents = m_context.getParents(*expr);
    for (const auto& parent : parents) {
        if (parent.get<clang::VarDecl>()) {
            return true;
        }
        // Also check if wrapped in another expression that's a VarDecl initializer
        if (const auto* parentExpr = parent.get<clang::Expr>()) {
            const auto& grandParents = m_context.getParents(*parentExpr);
            for (const auto& grandParent : grandParents) {
                if (grandParent.get<clang::VarDecl>()) {
                    return true;
                }
            }
        }
    }

    // Skip if this is the condition of an if/while/for - wrapping would break syntax
    for (const auto& parent : parents) {
        if (parent.get<clang::IfStmt>() || parent.get<clang::WhileStmt>() ||
            parent.get<clang::ForStmt>() || parent.get<clang::DoStmt>() ||
            parent.get<clang::SwitchStmt>()) {
            return true;
        }
    }

    // Skip return statements - they're handled separately
    for (const auto& parent : parents) {
        if (parent.get<clang::ReturnStmt>()) {
            return true;
        }
    }

    // Collect all temporaries that will be destroyed
    std::vector<std::pair<std::string, unsigned>> temps;
    collectTemporaries(expr->getSubExpr(), temps);

    if (temps.empty())
        return true;

    // Build the ghost dtor calls - temporaries are destroyed in reverse order
    std::string ghostCalls;
    for (auto it = temps.rbegin(); it != temps.rend(); ++it) {
        ghostCalls += ", __inspector_ghost_dtor(\"" + it->first + "\", " +
                      std::to_string(it->second) + ")";
    }

    // Wrap the expression: (original_expr, __inspector_ghost_dtor(...), void())
    // We add void() at the end to make the comma expression evaluate to void
    // This helps avoid issues with the expression's value being changed
    m_rewriter.InsertText(expr->getBeginLoc(), "(", /*InsertAfter=*/true);
    m_rewriter.InsertTextAfterToken(expr->getEndLoc(), ghostCalls + ", (void)0)");

    return true;
}

} // namespace inspector
