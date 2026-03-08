# See++ Option 3 — PoC Backend

A minimal proof-of-concept for the LLVM-instrumentation approach: a Clang AST
plugin that rewrites C++ source so the resulting binary self-traces. Built and
run successfully against Clang/LLVM 18 on Ubuntu 24.04.

## What this proves end-to-end

The PoC produces a real JSON execution trace from a real instrumented binary.
For the included `test/example.cpp`, the output is 10 steps showing function
calls, returns, variable initializations, and reassignments with full stack
state at each step:

```
 0. call        main     | main     []
 1. var_init    a        | main     [a=3]
 2. var_init    b        | main     [a=3, b=4]
 3. var_init    sum      | main     [a=3, b=4, sum=7]
 4. var_update  sum      | main     [a=3, b=4, sum=8]
 5. call        square   | square   []
 6. var_init    result   | square   [result=64]
 7. return      square   | square   [result=64]
 8. var_init    sq       | main     [a=3, b=4, sum=8, sq=64]
 9. return      main     | main     [a=3, b=4, sum=8, sq=64]
```

Format is a simplified version of pgbovine's OPT trace format used by Python
Tutor and See++. A frontend that consumes the OPT format will consume this
with minor adapter glue.

## Architecture (3 artifacts, 2 build passes)

The plugin's `clang::Rewriter` modifies a TEXT BUFFER, not the AST that gets
lowered to IR. So a source-to-source instrumenter needs two clang invocations:

```
  user.cpp
     |
     |  Pass 1:  clang -fsyntax-only -fplugin=libSeePlugin.so user.cpp
     |           Plugin walks AST, writes user.cpp.instrumented.cpp
     v
  user.cpp.instrumented.cpp     (rewritten, with __see_* calls injected)
     |
     |  Pass 2:  clang++ -c user.cpp.instrumented.cpp
     v
  user.o
     |
     |  Pass 3:  clang++ user.o libsee_runtime.a -o user
     v
  user                           (self-tracing binary)
     |
     |  Run:    ./user 2> trace.json
     v
  trace.json                     (OPT-style execution trace)
```

This two-pass model is the standard pattern for source-rewriting plugins
(it's what clang-tidy fix-its do too). A future revision could be a single
pass via libtooling, or — more ambitiously — by injecting `CallExpr` AST
nodes directly so the rewrite participates in the same compilation, but
both are significantly more code than the two-pass version.

## Build & run

```
apt install llvm-18-dev libclang-18-dev clang-18 cmake
cmake -B build -DCMAKE_CXX_COMPILER=/usr/lib/llvm-18/bin/clang++
cmake --build build -j
CLANGXX=/usr/lib/llvm-18/bin/clang++ ./scripts/instrument-and-run.sh
```

## Files

- `plugin/SeePlugin.cpp` — Clang AST plugin. ~270 lines, single file.
- `runtime/see_runtime.{h,cpp}` — tracing runtime. ~250 lines, no deps.
- `test/example.cpp` — sample input that exercises calls/returns/init/update.
- `scripts/instrument-and-run.sh` — three-pass build driver.
- `CMakeLists.txt` — finds installed LLVM/Clang via their CMake configs.

## Known issues (deliberate, all flagged in source)

These are real bugs visible in the rewritten source. None block the PoC; all
are documented because they are useful learning artifacts.

1. **Dead `__see_leave` after every `return`.** The plugin injects the leave
   call both before each `return` (via `VisitReturnStmt`) and at the function's
   closing brace (via `VisitFunctionDecl`). The post-return leave is unreachable
   for any function that returns. Visible in the rewrite, harmless at runtime.
   Fix: track in a per-function flag whether the function falls off the end,
   and only emit the closing-brace leave when it does.

2. **`line: 0` in every event.** The `__see_step(line)` injection is
   intentionally disabled in the plugin. Naive injection before every `Stmt`
   breaks compound expressions like `if (cond) stmt;` (becomes
   `if (cond) __see_step(N); stmt;` — two statements where one was expected).
   The proper fix needs a parent-aware visitor that only injects at statements
   that are direct children of a `CompoundStmt`. Roughly 50 lines of additional
   visitor code.

3. **Only `int` is tracked.** Other primitives (long, double, char, bool) need
   one runtime hook each, or one variadic hook with a type tag. Pointers need
   a hook that records the address value. User-defined types need recursive
   field encoding via `RecordDecl::fields()`.

4. **No heap tracking.** `new` and `delete` aren't intercepted. The cleanest
   addition: visit `CXXNewExpr` in the AST (insert `__see_alloc(ptr, size, "T")`
   after the new), and provide an `__see_free` hook. The runtime would maintain
   an interval tree of live allocations so pointer-to-heap resolution works.

5. **No template handling.** Templates are visited at definition time, not
   instantiation time. `std::vector<MyType>` will not be instrumented
   per-instantiation. Real fix is dual instrumentation: AST for names, IR pass
   for events.

6. **`__see_var_update` only catches `=`.** `+=`, `-=`, `++` etc. are
   `BinaryOperator` / `UnaryOperator` nodes with different opcodes; not handled.

7. **No type encoding in trace.** Real OPT format encodes pointers as
   `["REF", heap_id]` and structs as ordered field lists. We emit raw int
   values. Mechanical to add once the runtime tracks types.

## Why this PoC matters for the larger discussion

Three things this exercise actually demonstrated, not just argued:

1. **The mechanic works.** Clang plugins can produce self-tracing binaries
   via source rewriting. The output is recognizably the right shape.

2. **The pitfalls are real and surface fast.** Two non-obvious things had to
   be discovered by trying:
   - `getParents()` returns `DynTypedNodeList` which is forward-declared
     in `ASTContext.h`; you need `ParentMapContext.h` for the full type.
   - `Rewriter` modifies text, not the AST that codegen sees, so the same
     clang invocation that runs the plugin produces an uninstrumented binary.
     This is a well-known gotcha and the source of much confusion online.

3. **The "3+ months for production" estimate stands.** Just the seven items
   in "Known issues" above each represent a few days to a week of careful
   work. Templates alone could easily eat a month if you take it seriously.
   What you see here is closer to 1% than 10% of a real implementation.

## Suggested next steps if you take this further

In rough order of leverage:

1. Fix the dead-leave bug (small, removes confusion in rewrites).
2. Implement parent-aware `__see_step` injection (biggest functional win:
   actual line numbers in the trace).
3. Add `CXXNewExpr` instrumentation + `__see_alloc` runtime hook.
4. Generalize the type-handling: separate hooks for primitive types, plus
   recursive struct/class encoding.
5. Implement pointer-to-heap resolution (interval tree of live allocations).
6. Add an LLVM IR pass for the things that do not survive at the AST level
   (stack lifetime markers, optimizer-resistant instrumentation).
7. STL via custom AST matchers for `std::vector`, `std::string`, `std::map`.
8. Drop in `nlohmann/json` and replace the hand-rolled JSON emitter.
9. Wrap the whole thing in a sandboxed runner (Docker, seccomp, timeout).
