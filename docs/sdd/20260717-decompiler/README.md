---
inherits: docs/sdd
spec_id: 20260717-decompiler
title: Decompiler (SCM bytecode → IR2)
status: approved
doc_role: slice-index
---

# Spec: Decompiler (SCM bytecode → IR2)

**Status:** approved

A decompiler / disasm subsystem for `gta3sc`: takes compiled SCM bytecode (a
`MAIN.SCM` multifile) and produces an
[IR2](reference-bytecode-format.md#ir2-output-format) textual disassembly. It
mirrors the compiler pipeline (`disasm/` ↔ `codegen/`, `DisasmIR`,
`driver::Decompilation`) and keeps a seam for future high-level uplifting
(out of scope for this spec).

**Source code is the source of truth.** This directory is disposable
scaffolding; humans may delete it anytime after the work is no longer needed.

## Documents (read in order)

| Order | File | Role |
|-------|------|------|
| 1 | [01-context](01-context.md) | Problem, goals, non-goals, constraints, glossary |
| 2 | [02-requirements](02-requirements.md) | Observable behaviour: inputs, IR2 output, diagnostics |
| 3 | [03-design](03-design.md) | Architecture, `DisasmIR` / `CodeDecoder`, layout, invariants |
| 4 | [04-plan](04-plan.md) | Touch points, implementation order, subtasks, sharp edges |
| 5 | [05-disasm-walk-and-events](05-disasm-walk-and-events.md) | `SegmentWalker`, event sink, `Ir2Printer` layering (delta) |

### Supporting docs

| File | Role |
|------|------|
| [reference-bytecode-format](reference-bytecode-format.md) | SCM header layout, datatype-byte table, IR2 grammar |
| [discussion-notes](discussion-notes.md) | Design-discussion context for future readers (not acceptance criteria) |

## Scope at a glance (decided)

- **Output:** IR2 text only; streamed via `std::function_ref<void(std::string_view)>`
  (**TODO:** revisit `ostream` / `LineSink` / …). Structured **event walk**
  underneath (see [05](05-disasm-walk-and-events.md)).
- **Versions:** GTA III & Vice City now; SA / custom headers deferred.
- **Strategy:** linear sweep now; recursive traversal pluggable later.
- **Header:** full III/VC multifile header + missions; headerless mode for
  tests/CLEO-later; SA streamed deferred.
- **Core:** `CodeDecoder` → transactional `DisasmIR` (no persistent decoded
  program). `SegmentAnalyzer` builds label + code/data metadata for classic
  IR2 labels — **not** required to use the decoder alone.
- **Lookup:** `InverseCommandTable` (`find_command` only), built from
  `CommandTable`.
- **Placement:** `disasm/` ↔ `codegen/`; `ir/disasm-ir.hpp`;
  `MultifileHeader` + `MultifileDisasm` in one `multifile-disasm` TU;
  `SegmentAnalyzer`, public `SegmentWalker` + event sink; `Ir2Printer` as sink;
  `driver::Decompilation` as thin façade.
- **CLI:** out of scope (stub remains a follow-up).
- **Diagnostics:** byte offsets via `FilePool`; printer formatting follow-up.

## Review gate

- [ ] Human sets spec `status` to `approved` (or waives in chat) before code.
- [ ] Subtasks (see [04-plan](04-plan.md)) have clear **Depends on** / acceptance.

## Follow-ups (explicitly not blocking this spec)

- IR2 (or alternate) offset-named labels → dump without analyze.
- Recursive traversal; SA; custom headers; high-level uplift.
- CLI wiring; diagnostic printer for bytecode offsets; output-sink revisit.
