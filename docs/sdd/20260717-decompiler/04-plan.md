---
inherits: docs/sdd
spec_id: 20260717-decompiler
title: Plan — Decompiler
status: approved
doc_role: stage-plan
---

# 4. Plan

How this repo realizes the requirements. Follow `AGENTS.md`: interface (`.hpp`)
→ unit tests (`unittest/`) → implementation (`.cpp`) → edge-case tests, for each
new API. `clang-format` and run the full `gta3sc_unittest` before landing.

## 4.1 Touch points

New:

- `include/gta3sc/inverse-command-table.hpp` + `lib/...`
- `include/gta3sc/ir/disasm-ir.hpp` (+ `lib/...` if needed)
- `include/gta3sc/disasm/trilogy/code-decoder.hpp` + `lib/...`
- `include/gta3sc/disasm/trilogy/multifile-disasm.hpp` + `lib/...`
  (`MultifileHeader`, `Segment`, `MultifileDisasm`)
- `include/gta3sc/disasm/segment-analyzer.hpp` + `lib/...`
- `include/gta3sc/disasm/segment-walker.hpp` + `lib/...` (`SegmentWalker`,
  `DisasmEventSink`)
- `include/gta3sc/disasm/trilogy/ir2-printer.hpp` + `lib/...`
- `include/gta3sc/driver/decompilation.hpp` + `lib/driver/decompilation.cpp`
- `unittest/disasm/*.cpp` (+ register in `unittest/CMakeLists.txt`)
- `lib/CMakeLists.txt` entries for the new sources

Modified (small, justified):

- `DESIGN.adoc` — add a "Decompiler" / disasm section.
- `BACKLOG.md` — tick decompiler items as they land.
- *(follow-up, out of scope)* `lib/cli/run-decompile.cpp` — replace the
  not-implemented stub once the driver exists.

## 4.2 Implementation order

1. **`InverseCommandTable`** — `from(CommandTable)`, `find_command` /
   `command_table()`, lifetime rules, tie-break tests.
2. **`DisasmIR` + `CodeDecoder`** — III/VC single-instruction decode; Builder /
   factories for printer unit tests; decoder tests against `CodeEmitter` bytes.
3. **`MultifileHeader` + segmentation** (in `multifile-disasm`) — III/VC header,
   mission split, headerless mode.
4. **`SegmentAnalyzer` (linear sweep)** — label sets, code/data map.
5. **`SegmentWalker` + `Ir2Printer`** — event walk; IR2 sink; `function_ref`
   lines (see [05](05-disasm-walk-and-events.md)).
6. **`MultifileDisasm` + `driver::Decompilation`** — wire library + driver;
   `FilePool` diagnostics-by-offset.
7. **CLI** — out of scope for this spec; leave the stub for a follow-up.
8. **`DESIGN.adoc`** decompiler/disasm section.

## 4.3 Subtasks

| ID | Name | Depends on | Notes |
|----|------|-----------|-------|
| 01 | `inverse-command-table` | — | `InverseCommandTable::from` + `find_command` + tests |
| 02 | `code-decoder` | 01 | `DisasmIR` + `CodeDecoder`; III/VC; bounds tests |
| 03 | `multifile-header` | — | Header parse + segment split + headerless (can parallel 01) |
| 04 | `segment-analyzer-linear` | 02 | Linear sweep, labels, code/data |
| 05 | `walker-and-ir2-printer` | 02, 04 | `SegmentWalker`, event sink, `Ir2Printer` |
| 06 | `multifile-disasm-and-driver` | 03, 04, 05 | `MultifileDisasm` + `driver::Decompilation` (CLI follow-up) |

Branch pattern: `gta3sc-rewrite-branches/20260717-decompiler/<name>`.

## 4.4 Testing approach

- **Unit** (doctest): `CodeDecoder` against buffers from `CodeEmitter` /
  hand-built bytes; `DisasmIR::Builder` for isolated printer formatting tests;
  header corrupt cases; `SegmentAnalyzer` label/code-data behaviour; walker /
  `Ir2Printer` exact IR2 lines.
- **Round-trip smoke** (informal): compile tiny `.sc` → decompile → inspect IR2.
- **Integration (lit):** blocked on CLI follow-up; out of scope here.

## 4.5 Sharp edges / risks

- III vs VC float / text-label datatype divergence (`DecodeOptions`).
- III/VC text labels without datatype byte — parameter-directed decode.
- Reverse-lookup collisions — document tie-break when implementing.
- Diagnostics by offset via `FilePool`; printer formatting is a follow-up.
- Do not copy legacy structure; use it only as a behavioural reference.

## 4.6 Out-of-scope follow-ups

- Recursive-traversal strategy.
- SA streamed scripts, IEEE floats, array/text-label-var datatypes, custom
  headers (separate specs).
- IR2 (or alternate) offset-named labels so analyze is optional for dump.
- Arena-materialized IR + high-level uplifting + GTA3script printer.
- CLI `decompile` wiring; diagnostic printer for bytecode offsets.
- Revisit output sink (`ostream` / `LineSink` / …).

## 4.7 Links

- Conventions: [`../../../AGENTS.md`](../../../AGENTS.md)
- Architecture: [`../../../DESIGN.adoc`](../../../DESIGN.adoc)
- CLI stub (follow-up): [`../20260625-cli/subtasks/06-decompile-stub.md`](../20260625-cli/subtasks/06-decompile-stub.md)
