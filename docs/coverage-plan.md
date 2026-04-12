# Teachable C++ Coverage Plan

This document captures the gap between the current C++ Runtime Inspector
golden suite and a complete intro→intermediate C++ teaching surface, plus
the staged plan to close that gap.

The audit that produced this plan is summarized in
[`supported-language-subset.md`](supported-language-subset.md). The
five-tier structure below sorts work by cost, not by importance.

## Tier 1 — Tests for already-supported features (~1 day) — **DONE**

The plugin and runtime already understand these constructs; we just
have no golden test that exercises them. Each item below has a
directory under `tests/golden/` with `input.cpp` plus an
`expected.json` produced by `scripts/update-expected.sh`.

- `overloading` — same name, different signatures
- `default_args` — `void f(int = 0, int = 1)`
- `const_correctness` — `const` parameters, `const` methods, `const` locals
- `constexpr` — compile-time evaluation of small functions
- `range_for` — `for (auto& x : container)`
- `auto_deduction` — `auto` for primitives, references, iterators
- `structured_bindings` — `auto [a, b] = pair`
- `enum_class` — scoped vs unscoped enum behavior in same program
- `function_pointers` — pointer to free function, called indirectly
- `static_members` — class-level static data + static method
- `namespaces` — nested namespaces, ADL
- `class_template` — `template <class T> struct Box { T v; };`
- `template_specialization` — full and partial
- `multi_catch` — `catch (A&)` then `catch (B&)`
- `rethrow` — `catch (...) { throw; }`
- `mutual_recursion` — `is_even`/`is_odd` calling each other
- `argc_argv` — read program arguments

### Caveats surfaced by the new tests

These are pre-existing instrumenter limitations the new tests
*expose* but do not *cause*. They are tracked under their natural
tier and do not block Tier 1 closure.

- `static_members`: `++this->self` inside `bump()` does not update
  `a.self` / `b.self` in the trace. The visitor handles `BinaryOperator`
  / `UnaryOperator` whose LHS is a `DeclRefExpr`, but a write through
  `this` is a `MemberExpr`, which is not instrumented. → Tier 3
  (method-body / `this` tracking).
- `structured_bindings`: each `BindingDecl` is captured, but the
  source `std::pair p` is skipped because pair has no runtime
  encoder yet. → Tier 2 (`std::optional` / `std::variant` /
  associative-container encoders include richer pair handling).
- `constexpr`: function bodies are deliberately untraced
  (instrumenting them would break the constexpr contract). The
  trace still shows compile-time-evaluated values and the runtime
  result of constexpr functions called with non-constexpr arguments.
  Not a gap; a design choice.

### Plugin / runtime fixes Tier 1 required

- `Hooks` and `TraceState` now take `const void* addr` so `const`
  locals can be captured.
- `TraverseFunctionDecl` (and `CXXMethodDecl` / `Constructor` /
  `Destructor` / `ConversionDecl`) skip constexpr bodies.
- `ensureCompoundBody` advances past a trailing `;` before placing
  the closing brace; the `__inspector_leave` / `__inspector_throw`
  / operator wrappers use `InsertAfter=true` so they sit *inside*
  any synthetic braces.
- `DecompositionDecl` is unrolled into one `__inspector_var_init`
  per `BindingDecl`.
- `VisitCXXForRangeStmt` injects an init call for the loop variable
  at the top of each iteration body.
- `VisitFunctionDecl` and `findEnclosingFunctionName` use
  `getQualifiedNameAsString()` so `func_name` shows
  `math::square`, `Counter::bump`, `Box::get`, etc.
- Test harness honors a per-test `.args` file.

Definition of done: 17 new green goldens, total suite ≥ 51 tests, two
consecutive `run-golden-tests.sh` runs pass with no diff.

## Tier 2 — Runtime encoders + minor plugin work (~2–3 days) — **DONE**

Each item below has a green golden under `tests/golden/`.

- **`std::map` / `std::set`** — `encodeStdMap` walks the libstdc++
  red-black tree using header offsets at `(size − 32)`, in-order
  iteration via parent pointers, capped at 256 entries.
  Goldens: `stl_set`, `stl_map` (keys only — value-type plumbing
  for `pair<const K, V>` is a follow-up).
- **`std::shared_ptr`** — golden `stl_shared_ptr` exercises shared
  ownership + `reset`.
- **`std::optional`** — `encodeStdOptional` reads engaged byte at
  offset `sizeof(T)` and decodes payload via `state.encodeValue`.
  Golden `stl_optional`.
- **`std::variant`** — `encodeStdVariant` reads discriminator at
  `(size − 8)`. Decodes payload only when index == 0
  (TypeDescriptor only carries one element_type; multi-alternative
  decode needs a second descriptor field). Golden `stl_variant`.
- **`std::function`** — `encodeStdFunction` reports `engaged` flag
  via libstdc++'s `_M_invoker` / `_M_manager` slots. Golden
  `stl_function`.
- **Bitfields** — `FieldInfo` extended with `is_bitfield`,
  `bit_offset`, `bit_width`. Plugin populates them via
  `field->getBitWidthValue` / `getFieldOffset`. Runtime extracts the
  value by shifting and masking the surrounding bytes, with
  sign-extension for signed bitfields. Golden `bitfields`.
- **Unions** — golden `unions` (the encoder existed already; this
  required member-write tracking, which was added in Tier 2).

### Side effects of Tier 2

- `__inspector_var_init_struct` and `var_update_struct` now route
  STL containers to `encodeValueAtAddress` so their dedicated
  encoders run. Without this, a top-level STL local was rendered as
  a generic field walk over libstdc++ private members.
- `VisitVarDecl` no longer skips STL containers — they emit their
  init call so the local variable is visible in the trace.
- `VisitBinaryOperator` recognises member-expression LHS
  (`x.field = …`) so writes through structs/unions update the trace.
- `emitStep` re-encodes every local from its address before
  snapshotting. Without this, mutations through method calls or
  CXXOperatorCallExpr (e.g. `vector::push_back`,
  `variant::operator=`, `optional::reset`) did not appear in the
  trace because the visitor only instruments built-in
  `BinaryOperator` / `UnaryOperator` writes.
- `getStlElementType` now drills into `TemplateArgument::Pack` so
  variadic templates (`variant<Ts...>`) report a concrete first
  argument.
- The descriptor generator skips field/base walks for STL types,
  preventing libstdc++ implementation types like `_Rb_tree_node_base`
  from being pulled into the descriptor table.

## Tier 3 — New plugin AST visitors (~3–5 days) — **DONE**

Each item below has a green golden under `tests/golden/`.

- **`this->member` write tracking** — `VisitBinaryOperator`,
  `VisitCompoundAssignOperator`, `VisitUnaryOperator` recognize
  `MemberExpr` whose innermost base is a `CXXThisExpr` and wrap the
  write with a post-mutation `__inspector_step` call. Live re-encoding
  in `emitStep` snapshots every stack frame, so the receiver living in
  a caller's frame is updated. Closes the Tier 1
  `static_members` caveat about `++this->self`.
  Goldens: `this_member_writes`, plus richer `static_members` and
  `class_template` traces.
- **Constructor / destructor instrumentation** — ctors/dtors are
  `FunctionDecl`s, so `VisitFunctionDecl` already handles entry/exit
  events with qualified names like `Guard::Guard` and `Guard::~Guard`.
  The first snapshot inside the body fires *after* the member-init
  list runs, so fields are already populated. Goldens: `raii`,
  `ctor_dtor_lifecycle` (covers default/copy/move ctors plus
  copy/move assignment plus dtor).
- **User-defined operator overloads** — operator overloads are also
  `FunctionDecl`s and instrument identically. The
  `wrapThisWriteWithStep` path covers `this->x += n` etc. inside
  the operator body. Golden: `operator_overload` (binary +, +=,
  pre/post ++, free-function unary -, member ==).
- **Virtual dispatch + dynamic type tagging** — when encoding a
  polymorphic struct, `Trace::encodeStruct` reads the vtable pointer
  at offset 0 and resolves it to a class name via `dladdr` +
  `abi::__cxa_demangle` (`runtime/inspector/Dynamic.cpp`). The result
  is emitted as a 4th array element after the field map:
  `["C_STRUCT", "Shape", {fields}, "Circle"]` — backward-compatible
  for non-polymorphic types. Goldens: `virtual_dispatch` (call through
  `Shape*` reaches `Circle::area`/`Square::area`), `abstract_class`
  (pure virtual + polymorphic `delete` through base pointer).
- **Richer lambda captures** — the synthesized fields of a lambda
  closure type carry empty source-level names; we now look them up via
  `CXXRecordDecl::getCaptureFields` and re-key them by the capturing
  variable's name. Reference captures are decoded properly because
  `encodePrimitive` learned a `TypeKind::Reference` case (it was
  silently returning 0 before). Struct-by-value captures are
  recursively encoded, so nested fields are visible. Goldens: refreshed
  `lambda` plus new `lambda_captures` (primitive value + primitive ref
  + struct value + struct ref + `[=]` + `[&]`).

### Plugin / runtime fixes Tier 3 required

- `wrapThisWriteWithStep` helper: wraps `(write, __inspector_step(line))`
  so a step fires post-write and live re-encoding picks up the change
  in every frame, including the caller's where the receiver actually
  lives.
- Insertion-point selection now keeps the *earliest* TU-scope position
  (computed via `SourceManager::isBeforeInTranslationUnit`); for
  methods we walk to the enclosing `CXXRecordDecl` and, for class
  templates, up once more to the `ClassTemplateDecl`. Without this, a
  test with member functions that reference type descriptors (e.g.
  `operator_overload`) would emit references before the descriptor's
  declaration.
- VisitBinaryOperator/Compound/Unary now emit a step (not a
  var_update) when the LHS is a non-local var of composite/reference
  type — that covers global struct member writes and reference
  parameters whose types we never emit a descriptor for.
- `Trace::encodeStruct` skips dynamic-type resolution unless
  `is_polymorphic` is set, so non-polymorphic structs are unchanged.
- `Trace::encodePrimitive` now handles `TypeKind::Reference` the same
  way as `Pointer` (read the slot as a pointer and dispatch through
  `encodePointer`). Reference-typed struct fields previously hit the
  default branch and rendered as `0`; this fix flowed through
  `auto_deduction`, `comprehensive`, `mixed_types`, and `references`,
  which were regenerated.
- `emitStep`'s heap-snapshot loop now copies `Allocation` values into
  a local vector before iterating. `std::map` insert in the loop body
  triggers operator `new` → `malloc` → the LD_PRELOAD shim →
  `HeapTracker::insert` → potential `m_allocations.push_back` resize,
  which previously invalidated the `const Allocation*` pointers
  handed back by `getLiveAllocations()`. The bug was latent before
  Tier 3; growing `StructValue` shifted timing and made it
  deterministic.

Definition of done: 7 new green goldens (`this_member_writes`,
`raii`, `ctor_dtor_lifecycle`, `operator_overload`, `virtual_dispatch`,
`abstract_class`, `lambda_captures`), suite size 66/66 deterministic
across three consecutive runs.

## Tier 4 — Semantic concept work (~1–2 weeks)

Needs design choices, not just plumbing.

- **Rule-of-5 lifecycle tracking** — distinguish copy-ctor vs move-ctor
  vs assignment vs default events; "ghost" frame for temporary
  destruction order. Requires a lifecycle event model in the trace
  schema.
- **Iterator visualization** — render "iterator pointing at element 3
  of vector v" without it looking like a raw pointer.
- **Multi-file programs** — instrumenter writes one
  `.instrumented.cpp` per TU; runtime symbol uniqueness across TUs
  (string-pool dedup, ODR-safe globals) needs auditing. Build script
  changes to compile/link multiple files.
- **`dynamic_cast` / `typeid`** — currently out of scope in
  `supported-language-subset.md`; would require turning RTTI back on.

## Tier 5 — Out of scope

Excluded by the existing scope doc; recommend keeping excluded:
threading, coroutines, concepts, modules, file I/O, custom allocators,
placement new.

## Recommended order

1. Tier 1 (cheap, payoff = "the docs are honest"). **Done.**
2. Tier 2 — STL encoders + bitfields. **Done.**
3. Tier 3 — ctor/dtor visibility, operator overloads, virtual dispatch
   with dynamic-type tagging. **Done.**
4. Tier 4's rule-of-5 lifecycle, then iterators, then multi-file.

Tracked in this file; tick off as tiers complete.
