---
inherits: docs/sdd
spec_id: 20260717-decompiler
title: Requirements — Decompiler
status: approved
doc_role: stage-requirements
---

# 2. Requirements

Observable behaviour and contracts. Concrete module/file layout is in
[04-plan](04-plan.md); the byte-level and IR2 grammar details live in the
[reference](reference-bytecode-format.md).

## 2.1 Inputs

- **R-IN-1** The decompiler accepts a compiled SCM byte buffer
  (`std::span<const std::byte>`), an **`InverseCommandTable`** (opcode →
  `CommandDef*`, built from a `CommandTable` and aliasing its defs), a target
  **version** (III or VC for now), and a `DiagnosticHandler`.
- **R-IN-2** A target version selects decode behaviour that must be the exact
  inverse of the encoder for that version (float encoding, presence of
  text-label datatype bytes, etc. — see [reference](reference-bytecode-format.md#datatype-bytes)).
- **R-IN-3** A **headerless** mode decodes the buffer as a single code segment
  starting at offset 0 (used for tests, fragments, and later CLEO-style scripts
  that are not wrapped in a multifile SCM header). The default mode parses the
  SCM header (R-HDR-*).
- **R-IN-4** The `InverseCommandTable` is consulted to map opcode → command and
  to determine per-parameter decode where the bytecode is not self-describing
  (III/VC 8-byte text labels). SA array/text-label-var datatypes are recognised
  by the decoder's type table but need not be produced this phase.

## 2.2 Header parsing

- **R-HDR-1** Parse the multi-segment SCM header for III/VC: the segment chain
  (via the leading `GOTO`s), global-variable space size, the **models** table,
  the **main** code size, and the **mission** offsets table. Layout in the
  [reference](reference-bytecode-format.md#scm-header).
- **R-HDR-2** Split the buffer into a **main** segment and one segment per
  **mission**, each with a correct base offset and size.
- **R-HDR-3** A malformed/truncated header produces an `error` diagnostic and
  aborts decompilation (no partial output claiming success).
- **R-HDR-4** *(SA, deferred)* Streamed-script segments and the associated
  `script.img` are recognised by the design but not decoded now. Encountering an
  SA-only header shape yields a clear "unsupported target" diagnostic rather
  than silent misparse.

## 2.3 Instruction decoding (`CodeDecoder` → `DisasmIR`)

- **R-DEC-1** Given a segment and an offset, `CodeDecoder::next()` is
  **transactional**: it either returns a complete `DisasmIR` (opcode / command
  id, *not flag*, resolved `CommandDef`, and a full argument sequence) or
  `nullopt`. There is no half-built instruction. See
  [05](05-disasm-walk-and-events.md) / [discussion-notes](discussion-notes.md).
- **R-DEC-2** Each argument decodes to one of: `int8`, `int16`, `int32`,
  `float`, global variable, local variable, 8-byte text label, 128-byte string,
  end-of-argument-list (EOAL), and (recognised, SA-deferred) array / text-label
  variable / 16-byte / variable-length string forms. String/text bytes may be
  exposed as views into the segment buffer (no copy of those bytes required).
  How args are stored inside a successful `DisasmIR` is an implementation
  detail; keep allocations/fragmentation under control.
- **R-DEC-3** Decoding **must not read out of bounds**; a fetch past the segment
  end fails gracefully (`CodeDecoder` reports a decode failure, never crashes or
  reads adjacent memory). This mirrors the safety of the legacy `BinaryFetcher`.
- **R-DEC-4** An instruction whose opcode is unknown, or whose argument stream
  is inconsistent with the command's parameters, is a **decode failure** at that
  offset (not a hard abort). There is no "unknown command" IR node: those bytes
  become part of an `IR2_HEX` run (R-DIS-3). The decoder reports failure; the
  analyzer / walker / printer decide how far the undecoded run extends.
- **R-DEC-5** `CodeDecoder` performs **no control-flow analysis** and holds
  **no per-program state** beyond the current cursor — it is the semantics-free
  read counterpart of `CodeEmitter`.

## 2.4 Disassembly / analysis

- **R-DIS-1** **Linear sweep** decodes a segment from its start offset to its
  end, one instruction after another.
- **R-DIS-2** Branch/label arguments (parameters typed `LABEL`) contribute
  **label targets**: a non-negative value is an offset into the *main* segment;
  a negative value is a *segment-local* offset (its absolute value). Targets are
  collected so every referenced location gets a label definition.
- **R-DIS-3** Bytes that do not decode as an instruction are preserved as a
  **hex data run** (`IR2_HEX`). Granularity is **per byte**, matching legacy:
  when decode fails at offset `O`, analysis treats `O` as non-code and may
  retry at `O+1`; when printing, every consecutive non-code byte becomes one
  signed `i8` token in `IR2_HEX …` (e.g. `IR2_HEX 12i8 -1i8 0i8`). A label
  offset may split a hex run. No bytes are dropped.
- **R-DIS-4** The analysis output is **compact metadata** (the set of label
  offsets per segment and the code/data partition), *not* a materialized decoded
  program. The **walker** re-drives `CodeDecoder` at code offsets; the printer
  formats events to IR2. Analysis is required for classic IR2 label ids; it is
  **not** required to use `CodeDecoder` alone.
- **R-DIS-5** The strategy is selectable; **recursive traversal** is a planned
  alternative that follows branch targets to classify code vs data. Only linear
  sweep is required now, but the strategy interface must not assume linear sweep.

## 2.5 IR2 output

Full grammar: [reference](reference-bytecode-format.md#ir2-output-format).
Requirements on the emitted text:

- **R-OUT-1** Output is a sequence of lines; each line is a **directive**, a
  **label**, or a **command** (incl. the `IR2_HEX` pseudo-command). No blank
  lines; no leading/trailing whitespace; tokens separated by a single space.
- **R-OUT-2** Header info is emitted as directives: `#DEFINE_MODEL <NAME> -<i>`
  for each model; mission blocks are delimited by
  `#MISSION_BLOCK_START <id>` / `#MISSION_BLOCK_END`. (`#DEFINE_STREAM` /
  `#STREAMED_BLOCK_*` are SA, deferred.)
- **R-OUT-3** Labels are named `MAIN_<id>` (main segment) and
  `MISSION_<block>_<id>` (mission segments); `<id>` starts at 1 per block and
  increases in offset order. Label *references* use `@NAME` when they point into
  the main segment and `%NAME` when they point into the current segment.
- **R-OUT-4** Integer arguments carry an explicit width suffix reflecting how
  they were encoded: `…i8`, `…i16`, `…i32` (no leading zeros).
- **R-OUT-5** Floats are emitted as normalized C99 hex-float constants with a
  6-digit fraction and an `f` suffix (e.g. `0x1.000000p+0f`), decoded from the
  version's float encoding.
- **R-OUT-6** Variables use the IR2 var syntax (`&<n>` global, `<n>@` local,
  with `s`/`v` affixes for text-label types). Strings/text-labels use the IR2
  string forms (`'…'`, `v'…'`, `b"…"`, `"…"`). Array forms per the reference
  (SA-deferred in output but grammar reserved).
- **R-OUT-7** A command line begins with `NOT ` iff the not flag is set,
  followed by the command name, followed by space-separated arguments; the EOAL
  terminator produces no visible token.
- **R-OUT-8** Output is **deterministic**: identical input bytes + version +
  `InverseCommandTable` contents ⇒ byte-identical IR2.
- **R-OUT-9** Output is streamed line-by-line to a
  `std::function_ref<void(std::string_view)>` sink (or equivalent), so large
  scripts need not be buffered whole.
  **TODO:** revisit whether `ostream`, `LineSink`, or another abstraction is
  preferable for some callers.

## 2.6 Diagnostics

- **R-DIAG-1** Recoverable problems are reported through `DiagnosticHandler`
  and reference the **byte offset** at which they occur (unknown opcode,
  undecodable region, branch target outside the segment).
- **R-DIAG-2** Unrecoverable problems (corrupt header, unsupported target) are
  reported as `error` and cause `decompile` to return failure.
- **R-DIAG-3** In linear-sweep mode, an unknown opcode does **not** abort: it is
  reported (warning/info) and the region becomes `IR2_HEX`. Analysis continues.
- **R-DIAG-4** No diagnostic path may depend on GTA3script source text; there is
  none.

## 2.7 CLI

Out of scope for this spec. The library/driver API must be callable from a
future `cli::run_decompile`, but wiring the stub is a separate follow-up.

## 2.8 Success criteria (behavioural)

- **R-OK-1** A small hand-authored III/VC `MAIN.SCM` (produced by the existing
  compiler) decompiles to IR2 that, per instruction, names the correct command,
  argument types, widths, and values.
- **R-OK-2** Unknown/garbage bytes never crash and never silently vanish — they
  surface as `IR2_HEX` plus a diagnostic.
- **R-OK-3** Label references resolve to the right `@MAIN_n` / `%MISSION_b_n`
  names and every referenced offset has a definition.
