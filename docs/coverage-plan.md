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

## Tier 3 — New plugin AST visitors (~3–5 days)

Add to `plugin/Visitor.cpp` plus matching runtime hooks:

- `VisitCXXConstructorDecl` / `VisitCXXDestructorDecl` — emit
  `ctor_enter` / `dtor_enter` events. Unlocks RAII visualization, which
  is the biggest pedagogical hole.
- `VisitCXXOperatorCallExpr` — instrument user-defined operator
  overloads (current visitor only handles built-in operators).
- `VisitCXXMemberCallExpr` annotated with virtual-dispatch flag — show
  dynamic vs static type at virtual call sites. Unlocks polymorphism
  teaching.
- Richer `VisitLambdaExpr` — capture struct/STL captures, not just
  primitives.

Definition of done: goldens for `ctor_dtor_lifecycle`, `raii`,
`operator_overload`, `virtual_dispatch`, `abstract_class`.

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

1. Tier 1 (cheap, payoff = "the docs are honest").
2. Tier 3's ctor/dtor support (highest pedagogical value).
3. Tier 2's `map`/`set`.
4. Remainder of Tier 2.
5. Tier 4's rule-of-5 lifecycle, then iterators, then multi-file.

Tracked in this file; tick off as tiers complete.
