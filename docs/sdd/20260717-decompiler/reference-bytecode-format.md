---
inherits: docs/sdd
spec_id: 20260717-decompiler
title: Reference — SCM bytecode & IR2 format
status: approved
doc_role: reference
---

# Reference: SCM bytecode & IR2 format

Authoritative decode/emit details for the decompiler. Behaviour is cross-checked
against the legacy `src/disassembler.cpp` / `src/decompiler_ir2.hpp`
(`~/dev/gta3sc`, `master`) and the compiler's `codegen::trilogy::CodeEmitter`
(the encoder we invert). External refs: <https://gtamods.com/wiki/SCM_Instruction>
and the IR2 proposal gist by @thelink2012.

> Scope reminder: III/VC now; SA rows below are documented for completeness and
> to size the decode tables, but SA decoding is deferred.

## Instruction layout

```
[ u16 opcode ][ arg ][ arg ]... [ 0x00 EOAL if command has optional params ]
```

- `opcode & 0x8000` → **not flag**; `opcode & 0x7FFF` → **command id**.
- The command id is resolved to a `CommandDef` via `InverseCommandTable`
  (design §3.8). Its parameters drive decode where the stream is not
  self-describing (III/VC 8-byte text labels).
- A command with an optional/variadic tail is terminated by a `0x00` datatype
  byte (EOAL), matching `CodeEmitter::emit_eoal()`.

## Datatype bytes

The byte preceding most arguments. Table mirrors legacy `explore_opcode` /
`opcode_to_data`:

| Byte | Meaning | Payload | Status |
|------|---------|---------|--------|
| `0x00` | EOAL (end of argument list) | — | now |
| `0x01` | int32 | `i32` | now |
| `0x02` | global int/float var | `u16` offset | now |
| `0x03` | local int/float var | `u16` index | now |
| `0x04` | int8 | `i8` | now |
| `0x05` | int16 | `i16` | now |
| `0x06` | float | III: `i16` Q11.4 (`/16`); VC/SA: IEEE-754 `u32` | now (per-version) |
| `0x07` | global int/float array | `u16`+`i16`+`u8`+`u8` | SA (deferred) |
| `0x08` | local int/float array | same | SA (deferred) |
| `0x09` | immediate 8-byte string | 8 bytes | SA text-label (deferred) |
| `0x0A` | global text-label var | `u16` | SA (deferred) |
| `0x0B` | local text-label var | `u16` | SA (deferred) |
| `0x0C` | global text-label array | array | SA (deferred) |
| `0x0D` | local text-label array | array | SA (deferred) |
| `0x0E` | immediate variable-length string | `u8 len` + bytes | SA/CLEO (deferred) |
| `0x0F` | immediate 16-byte string | 16 bytes | SA (deferred) |
| `0x10` | global text-label16 var | `u16` | SA (deferred) |
| `0x11` | local text-label16 var | `u16` | SA (deferred) |
| `0x12` | global text-label16 array | array | SA (deferred) |
| `0x13` | local text-label16 array | array | SA (deferred) |

**III/VC special case (no datatype byte):** when a datatype byte `> 0x06` is
seen and the target lacks a text-label prefix (`DecodeOptions.has_text_label_datatype
== false`), and the current command parameter is `TEXT_LABEL`, there is **no**
datatype byte: back up one byte and read **8 raw chars** as an 8-byte text label.
This is why decode is command-parameter directed for III/VC.

**Array payload (SA, deferred):** `u16 var_offset`, `u16 index_var`,
`u8 array_size`, `u8 array_prop` where `prop & 0x7F` is elem type
(0=int,1=float,2=text-label,3=text-label16) and `prop & 0x80` marks a global
index. Local offsets are stored `/4` (multiply by 4 on decode), matching the
compiler's `emit_lvar` / `emit_var` (globals stored as `4*index`).

**Variable offsets:** globals are byte offsets (compiler emits `4 * var_index`);
locals are indices (`u16`). Legacy stores local `DecompiledVar.offset` as
`index * 4` and prints `offset / 4` — the printer must undo the scaling to show
the human index.

## SCM header

III/VC header is a chain of segments, each introduced by a `GOTO`
(`02 00 01` + `i32 target`). `from_bytecode` reads:

```
seg1 @ 0
seg2 = u32 @ seg1+3        # globals end / segment 2 start
seg3 = u32 @ seg2+3
seg4 = u32 @ seg3+3        # code start (III/VC)
global_vars_size = seg2

# models (segment 2):
num_models = max(1, u32 @ seg2+8) - 1
models[i]  = 24 chars @ seg2+8+4+24 + 24*i     # index 0 skipped (self)

# main + missions (segment 3):
main_size    = u32 @ seg3+8
num_missions = u16 @ seg3+8+8
mission_offsets[i] = u32 @ seg3+8+8+4 + incr + 4*i   # incr = 4 on SA, else 0
```

SA extends the chain (seg5..seg7, `code_offset = seg7`) and adds a streamed
scripts table — **deferred**.

**Segmentation:** the *main* segment is `[code_offset, main_size)`; each mission
is `[mission_offsets[i], next_sorted_offset)` (last runs to end of buffer). A
mission offset `< main_size` or unsorted/overrunning ⇒ corrupt header
(error, abort).

## Custom headers — deferred

Some compiled scripts may carry custom headers ahead of the main code segment
(CLEO and related tooling). Handling those is out of scope for this decompiler
spec and will be covered by a separate, compiler-wide custom-header spec later.
Until then, encountering an unrecognized custom-header shape should produce a
clear unsupported/corrupt diagnostic rather than a silent misparse.

## IR2 output format

Machine-oriented, line-based. One line = directive, label, or command. No blank
lines, no boundary whitespace, single-space token separation.

### Directives (this phase)

| Directive | Meaning |
|-----------|---------|
| `#DEFINE_MODEL <NAME> -<INT>` | one per model, `<NAME>` uppercase |
| `#MISSION_BLOCK_START <INT>` / `#MISSION_BLOCK_END` | delimit a mission block |

Deferred (SA): `#DEFINE_STREAM`, `#STREAMED_BLOCK_START/END`.

### Labels

- Definitions: `MAIN_<id>:`, `MISSION_<block>_<id>:` — `<id>` from 1 per block,
  in offset order.
- References: `@NAME` when the target is in the **main** segment, `%NAME` when it
  is in the **current** segment.

### Argument tokens

| Kind | Pattern | Example |
|------|---------|---------|
| int8 / int16 / int32 | `-?[0-9]+i8|i16|i32` (no leading zeros) | `0i8`, `400i16`, `400000i32` |
| float | normalized C99 hex-float, 6 frac digits, `f` suffix | `0x1.000000p+0f` |
| global label ref | `@[_A-Z][_A-Z0-9]*` | `@MAIN_1` |
| local label ref | `%[_A-Z][_A-Z0-9]*` | `%MISSION_0_3` |
| global var | `[sv]?&[0-9]+` | `&8`, `s&8`, `v&8` |
| local var | `[0-9]+@[sv]?` | `0@`, `0@s`, `0@v` |
| array (SA, deferred) | `base(index,size,type)` | `v&16(0@,4v)` |
| 8-byte text label | `'[\x20-\x7E]*'` | `'TEXT'` |
| 16-byte text label (SA) | `v'…'` | `v'TEXT'` |
| 128-byte buffer | `b"…"` | `b"TEXT"` |
| string | `"…"` | `"TEXT"` |

Var type affix: none = int/float, `s` = text-label, `v` = text-label16.

### Commands & pseudo-commands

- `NOT ` prefix iff the not flag is set, then command name, then space-separated
  args. EOAL emits no token.
- `IR2_HEX <i8> <i8> …` preserves undecodable byte runs. Each token is one
  **byte** as a signed 8-bit integer (`12i8`, `-1i8`, …). Hex runs are
  byte-granular: a failed decode at offset `O` does not keep a phantom
  "unknown opcode" word; the opcode bytes (and any following undecodable
  bytes) are emitted as individual `i8` tokens until the next code region or
  label.

### Float emission detail

Decode the argument to a `float` (III: from Q11.4 `i16/16`; VC/SA: reinterpret
`u32`), then format as a normalized hex-float with a 6-hex-digit fraction and `f`
suffix (legacy used `printf("%.6af", value)`).
