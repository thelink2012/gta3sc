---
inherits: docs/sdd
spec_id: 20260717-decompiler
title: Design — Decompiler
status: approved
doc_role: stage-design
---

# 3. Design

## 3.1 Guiding idea

The compiler is a pipeline of small phases, each owning a problem, wired by a
driver (`driver::Compilation`). The decompiler is the **mirror image** and is
built the same way:

```
compiler:     text ─▶ ParserIR ─▶ (lower) ─▶ SemaIR ─▶ (lower) ─▶ bytecode
decompiler:   bytecode ─▶ MultifileHeader ─▶ SegmentAnalyzer ─▶ SegmentWalker ─▶ Ir2Printer
                              │                    │                  │
                              └──── Segment ───────┴── CodeDecoder (→ DisasmIR)
```

`CodeDecoder` is a shared primitive: the analyzer and walker use it; it is also
usable **without** analysis for non-IR2 consumers.

The compiler's back end (`codegen/trilogy/`) turns IR into bytes with a raw,
semantics-free `CodeEmitter` wrapped by IR-driven codegen. The decompiler's
front end turns bytes into views with a raw, semantics-free **`CodeDecoder`**
(the read counterpart of `CodeEmitter`), plus optional analysis and an IR2
printer.

The primary IR is **lazy**: `CodeDecoder` yields a transient **`DisasmIR`**
view of one instruction at a time, decoded straight from the byte cursor. No
persistent decoded instruction container is built for the print path. Analysis
retains only **offset metadata** (label sets, code/data partition) — and only
when a consumer needs it (classic IR2 label ids).

**`CodeDecoder` is usable alone.** Analysis is required for classic IR2 label
numbering (`MAIN_1`, …), not to decode instructions.

## 3.2 Module placement

### Alternatives considered

1. **`disasm/` as the counterpart of `codegen/`** *(chosen)* — target-specific
   pieces under `disasm/trilogy/`; IR types in `ir/`; driver in `driver/`.
2. **`decompiler/` top-level** — fine naming, but weaker symmetry with
   `codegen/`.
3. **Fold into `codegen/trilogy/`** — rejected: reading is not generation.
4. **Flat monolithic module** — rejected: blurs trilogy vs analysis vs print.

### Chosen layout

```
include/gta3sc/
  inverse-command-table.hpp   # InverseCommandTable (from CommandTable; find-only)
  ir/
    disasm-ir.hpp             # DisasmIR (+ nested Command / Argument); mirrors parser-ir / sema-ir naming
  disasm/
    trilogy/
      code-decoder.hpp        # CodeDecoder ↔ CodeEmitter
      multifile-disasm.hpp    # MultifileHeader + Segment + MultifileDisasm
      ir2-printer.hpp         # Ir2Printer (DisasmEventSink → IR2 lines)
    segment-analyzer.hpp      # SegmentAnalyzer → SegmentAnalysis (linear now)
    segment-walker.hpp        # SegmentWalker + DisasmEventSink (public)
  driver/
    decompilation.hpp         # Thin façade over the library API
lib/... (mirrors include/)
```

Detail on walker / events / printer: [05-disasm-walk-and-events](05-disasm-walk-and-events.md).


| Decompile piece                         | Compile counterpart                                                                                                                    |
| --------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------- |
| `CodeDecoder`                           | `CodeEmitter`                                                                                                                          |
| `MultifileDisasm` (+ `MultifileHeader`) | `MultifileCodeGen` (header write + per-file gen)                                                                                       |
| `DisasmIR`                              | `ParserIR` / `SemaIR` (naming/nesting; not the same ownership)                                                                         |
| `SegmentAnalyzer`                       | *No direct twin* — closest is label/symbol discovery during parse/sema (source already names labels; bytecode must rediscover offsets) |
| `SegmentWalker` + event sink            | *No twin* (lazy disasm walk)                                                                                                           |
| `Ir2Printer`                            | *No twin* (compiler does not emit IR2); implements the event sink                                                                      |
| `driver::Decompilation`                 | `driver::Compilation`                                                                                                                  |


`MultifileHeader`, `Segment`, and `MultifileDisasm` live in **one**
`multifile-disasm.hpp` / `.cpp`. `MultifileDisasm` is the **library** entry for
whole-SCM work (non-driver callers use it). `driver::Decompilation` loads the
file, wires diagnostics/`FilePool`, and calls into that API.

## 3.3 `DisasmIR`

Lives in `ir/disasm-ir.hpp`. Naming and nesting follow `ParserIR` / `SemaIR`
(`DisasmIR::Command`, `DisasmIR::Argument`, optional `ArgumentView` / lazy
range) without forcing arena list ownership on the lazy path.

- **Not** `IntrusiveBidirectionalListNode` / `ArenaObj` for this phase.
- Decode success only — no HEX variant on `DisasmIR` (hex is analysis/print).
- Accessors: `opcode()`, `command_id()`, `not_flag()`, `command()` →
  `const CommandTable::CommandDef*` (null should not occur on a successful
  decode), `args()`, `begin_offset()`, `end_offset()`.
- Argument `Type` enum follows existing IR style (`INT`, `FLOAT`, `EOAL`,
  `GLOBAL_VAR`, … — UPPER_CASE, no abbreviations like `flt`).
- **`CodeDecoder` constructs views directly** in the production path.
- **`DisasmIR::Builder` (or small factories)** exists for unit tests that need
  a hand-built instruction without bytecode (e.g. printer formatting). Decoder
  tests still go through real bytes.

## 3.4 `CodeDecoder`

```cpp
namespace gta3sc::disasm::trilogy
{
struct DecodeOptions
{
    bool has_text_label_datatype{false};
    bool half_float{true}; // III: Q11.4; else IEEE-754
    // custom-header / CLEO knobs reserved (separate future spec)
};

/// Semantics-free read counterpart of CodeEmitter.
class CodeDecoder
{
public:
    CodeDecoder(std::span<const std::byte> segment,
                const InverseCommandTable& commands,
                DecodeOptions options) noexcept;

    [[nodiscard]] auto offset() const noexcept -> uint32_t;
    void seek(uint32_t offset) noexcept;
    [[nodiscard]] auto eof() const noexcept -> bool;

    /// Decodes one instruction at the cursor; advances on success.
    /// Transactional: complete DisasmIR or nullopt (unknown id, bad args,
    /// truncated). On failure reposition with seek. No "unknown command" IR —
    /// undecoded bytes are handled by the walk / IR2_HEX at print time.
    [[nodiscard]] auto next() -> std::optional<DisasmIR>;
};
}
```

Bounds-checked fetches; never reads outside the segment (R-DEC-3).
Arg storage inside a successful `DisasmIR` is an implementation detail; keep
allocations under control (see [discussion-notes](discussion-notes.md)).

## 3.5 `MultifileHeader` and `MultifileDisasm`

In `disasm/trilogy/multifile-disasm.hpp`:

- **`MultifileHeader`** — parse III/VC header; models; main size; mission
  offsets; split into `Segment`s. Layout:
  [reference](reference-bytecode-format.md#scm-header).
- **`Segment`** — kind (main/mission), block id, `base_offset` + `size`
  (no `span` in the value — avoids dangling if the parent buffer moves; re-eval
  if awkward). Bytes come from the parent buffer at use sites.
- **`MultifileDisasm`** — library orchestration: parse header (or headerless
  single segment), run analysis per segment, drive walk + IR2 printing. This is
  the non-driver entry point.

Headerless mode (single segment at offset 0) is supported for tests, fragments,
and later CLEO-style scripts.

## 3.6 `SegmentAnalyzer` (analysis)

`disasm/segment-analyzer.hpp` turns a `Segment` + `CodeDecoder` into
**`SegmentAnalysis`**: sorted label offsets and a code/data partition. It does
**not** store decoded instructions. Keep this type segment-scoped; future
analyses (CFG, etc.) should be separate types, not growth on this class.

- **Linear sweep** — implemented now.
- **Recursive traversal** — strategy seam for later (worklist + explored
bitset); same `SegmentAnalysis` shape.
- `CodeDecoder` does not depend on this type.

Label-target sign convention (R-DIS-2): non-negative → main segment; negative →
current segment (`-value` local). Cross-segment targets update the main
analysis so printers can emit `@MAIN_n` from any segment.

API shape: two out-parameters (or one result struct with both), not
return-one + sink-one:

```cpp
void analyze(const Segment& segment,
             SegmentAnalysis& segment_out,
             SegmentAnalysis& main_labels_out);
```

## 3.7 Walk, events, and IR2 printer

See **[05-disasm-walk-and-events](05-disasm-walk-and-events.md)** for the public
`SegmentWalker`, `DisasmEventSink` (`on_label` / `on_command` /
`on_undecoded_bytes`), and `Ir2Printer` as a sink. Summary:

- `MultifileDisasm` emits directives (`#DEFINE_MODEL`, `#MISSION_BLOCK_*`) via
  `emit_line` directly; the walker is segment-local.
- `Ir2Printer` is constructed **after** label-id maps exist (typically after
  analyzing main / all segments), with those maps in the constructor.
- Top-level output remains `function_ref<void(std::string_view)>` IR2 lines.

## 3.8 `InverseCommandTable`

```cpp
class InverseCommandTable
{
public:
    /// Builds a find-only opcode → CommandDef* index.
    /// Pointers alias defs owned by `commands`; this table must not outlive
    /// that CommandTable / its arena.
    static auto from(const CommandTable& commands) -> InverseCommandTable;

    [[nodiscard]] auto find_command(int16_t command_id) const noexcept
            -> const CommandTable::CommandDef*;

    /// Returns the command table this inverse index was built from.
    [[nodiscard]] auto command_table() const noexcept -> const CommandTable&;
};
```

- Decompile / decode take **`InverseCommandTable`**, not `CommandTable`.
- Tie-break when building: prefer `target_handled`; then a deterministic rule
  (document at implement time, e.g. first inserted / non-`_INTERNAL` if
  detectable).
- Future custom ordinal overlays (e.g. CLEO-related) sit *on top* of this table;
  not in this spec.

## 3.9 Driver: `driver::Decompilation`

Thin façade: register `.scm` in a **`FilePool`** (byte offset → `FileLoc`),
build/load `InverseCommandTable`, call `MultifileDisasm`, report diagnostics.

```cpp
namespace gta3sc::driver
{
class Decompilation
{
public:
    Decompilation(std::span<const std::byte> bytecode, /* version */,
                  const InverseCommandTable& commands, FilePool& file_pool,
                  DiagnosticHandler& diag);
    // Path-based overload may load into FilePool then call the span API.

    /// Streams IR2 lines to emit_line.
    /// TODO: revisit output surface (ostream, LineSink, …)
    bool decompile(std::function_ref<void(std::string_view)> emit_line);
};
}
```

Arenas: not required on the lazy path. Analysis uses ordinary containers for
offset sets.

## 3.10 Output sink

**Now:** `std::function_ref<void(std::string_view)>` as a **parameter** to
`decompile` / print (no owning `std::function` in a stored `Result`; no
whole-IR2 string buffer in the hot API).

**TODO:** revisit whether `ostream`, a small `LineSink` interface, or another
abstraction is better for some callers. Do not buffer entire IR2 outside tests.

## 3.11 Diagnostics

Register the input `.scm` in a `FilePool` and map byte offset → `FileLoc`.
Descriptors under `disasm::diag` (mirroring `codegen::diag`).

**Follow-up (out of scope):** diagnostic *printer* formatting for bytecode
offsets (e.g. `main.scm:0x1A2B`).

## 3.12 High-level decompilation (out of scope)

Not specified here. Chat/design intent only: optional later materialization of
an arena `LinkedIR`-style IR + `InstructionRewriter` uplifting. The low-level
core must not depend on that.

## 3.13 Key invariants

- **INV-1** `CodeDecoder` never reads outside its segment span.
- **INV-2** `CodeDecoder` / `DisasmIR` hold no program-global state; cross-
instruction state lives only in analysis metadata when analysis runs.
- **INV-3** No bytes dropped: every segment byte is covered by code, a label
position, or an `IR2_HEX` run (byte-granular).
- **INV-4** Decode is the exact inverse of `CodeEmitter` for the chosen version.
- **INV-5** IR2 output is deterministic for classic IR2 label mode.
- **INV-6** Analysis is not required to use `CodeDecoder`.

