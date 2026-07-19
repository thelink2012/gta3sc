---
inherits: docs/sdd
spec_id: 20260717-decompiler
title: Design discussion notes (context)
status: approved
doc_role: discussion-notes
---

# Discussion notes (context for future readers)

This file captures **design discussion** that informed the spec. It is **not**
acceptance criteria and may lag the code. Prefer [03-design](03-design.md) and
[05-disasm-walk-and-events](05-disasm-walk-and-events.md) for what to implement.

## Naming evolution

| Earlier draft | Settled |
|---------------|---------|
| `CodeReader` | `CodeDecoder` (read counterpart of `CodeEmitter`) |
| `RawInstruction` / `DecodedInstruction` | `DisasmIR` (`ir/disasm-ir.hpp`) |
| `decompiler/` top-level | `disasm/` ↔ `codegen/` |
| `Disassembler` (analysis type) | `SegmentAnalyzer` (segment-scoped; split new analyses later) |
| `ScmHeader` | `MultifileHeader` (with `MultifileDisasm` in one TU) |
| `on_hex` / `on_instruction` | `on_undecoded_bytes` / `on_command` |

## Why transactional `next()`?

Either the cursor holds a **complete** command or decode **failed**. Pros: one
failure channel; strong `DisasmIR` invariant; HEX/undecoded fallback is obvious.
Cons: failure may need `seek`; success path materializes args for that one
insn.

**Lazy** means “no list of all instructions in the segment,” not “validate args
when the printer iterates.” Arg layout inside `DisasmIR` is left to
implementation with a hard constraint: **control allocations**.

## Why not stream only `DisasmIR`?

Labels and undecoded runs are not on `DisasmIR`. A useful mid-layer is an
**event walk** (`on_label` / `on_command` / `on_undecoded_bytes`). Top-level
product API stays IR2 **lines** (`function_ref<void(string_view)>`). Tools that
need structure use the walker/sink (or `CodeDecoder` alone), not IR2 parsing.

## Visitor vs materialize vs events

- **`InstructionVisitor` + `LinkedIR`:** fits compiler IR and future uplift /
  rewriters. Needs materialization (including some node for undecoded runs if
  you want a single list).
- **Event sink:** enough for IR2 print without allocating a program; `Ir2Printer`
  is one sink.
- Materializing for IR2 alone was judged unnecessary; keep a seam for later
  high-level decompilation.

## Decode cost (IR2 path)

Typical path decodes each real instruction about **twice** (once in
`SegmentAnalyzer`, once in `SegmentWalker` / print). Arg iteration must not
force a third decode if args were already pulled into `DisasmIR`.

Asymptotically, for segment size \(n\): analyze + walk are \(\Theta(n)\) time;
space \(O(n)\) for a code/data map (e.g. bitset) plus \(O(L)\) labels. Double
decode is cheap relative to IR2 string formatting for SCM-sized instructions.

## `Segment`: `base_offset` + `size`

Avoid storing a `span` in the segment value (dangling if the parent buffer
moves). Parent `bytecode` span is threaded at walk/print time. Re-evaluate if
awkward.

## Label maps and `Ir2Printer` construction

Construct the printer **after** label ids exist (after analyzing main / all
segments), passing maps into the **constructor** — not a late
`set_main_label_ids`. Directives stay on `MultifileDisasm` via `emit_line`.

## `on_undecoded_bytes` payload (open)

Lean at implementation: `on_undecoded_bytes(local_offset, span)`. May rethink to
offset+length if the sink already holds the parent buffer.

## `DisasmEventSink` and vtables

Abstract sink is clear for the library surface. Vtable overhead noted; may
revisit template/`function_ref` sinks later.

## Output sink TODO

`function_ref<void(string_view)>` for IR2 now. Revisit `ostream` / `LineSink` /
etc. if needed.

## Runtime interpreter (curiosity only)

`CodeDecoder` + `DisasmIR` + `InverseCommandTable` could feed a fetch/decode
step of a bytecode interpreter. Analyzer / IR2 / walker are not required for
that. Out of scope for this spec.

## Follow-ups called out in discussion

- IR2 (or alternate) offset-named labels → dump without analyze.
- Recursive traversal; SA; custom headers; high-level uplift + materialize.
- CLI wiring; diagnostic printer for bytecode offsets.
- Align `DecodeOptions` with future codegen options.
- Sink vtable vs template; `on_undecoded_bytes` span vs indices (open at impl).
- Public `SegmentWalker` (settled: ship it).
