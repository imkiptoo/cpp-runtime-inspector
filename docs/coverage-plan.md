# Teachable C++ Coverage Plan

This document captures the gap between the current C++ Runtime Inspector
golden suite and a complete intro→intermediate C++ teaching surface, plus
the staged plan to close that gap.

The audit that produced this plan is summarized in
[`supported-language-subset.md`](supported-language-subset.md). The
five-tier structure below sorts work by cost, not by importance.

## Tier 1 — Tests for already-supported features (~1 day)

The plugin and runtime already understand these constructs; we just
have no golden test that exercises them. Each item below should become
a single directory under `tests/golden/` with `input.cpp` plus an
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

Definition of done: 17 new green goldens, total suite ≥ 51 tests, two
consecutive `run-golden-tests.sh` runs pass with no diff.

## Tier 2 — Runtime encoders, no plugin work (~2–3 days)

Plugin already identifies these types in `plugin/TypeEncoder.cpp`; the
runtime needs to decode them in `runtime/inspector/StlEncoders.cpp`.

- **`std::map` / `std::set`** — biggest piece. Walk the libstdc++/libc++
  red-black tree (`_M_t._M_impl._M_header`). Define a `tree`-shaped
  encoded value so the frontend can render it.
- **`std::shared_ptr`** — encoder field already exists; needs golden +
  ref-count display test.
- **`std::optional`** — engaged-flag + payload encoder.
- **`std::variant`** — active-alternative index + payload encoder.
- **`std::function`** — target-type display.
- **Bitfields** — encoder reads bit offsets from `TypeDescriptor`.
- **Unions** — show all members with active-member hint.

Definition of done: golden test for every container/wrapper above.

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
