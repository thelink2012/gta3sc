# Bytecode diff methodology

How to locate the first semantic divergence when brute-force integration output
MD5 ≠ legacy. Applies to GTA III (`--config=gta3`) and Vice City (`--config=gtavc`).

Fixture paths and golden hashes: [`brute-force-summary.md`](brute-force-summary.md).

---

## Reference compile

Compile the same fixture with **legacy** `gta3sc` (see `gta3sc/test/main/gta3.test`
or `gtavc.test` for flags):

```bash
# GTA III
legacy_gta3sc build/gta3-test/main.sc --config=gta3 -pedantic-errors -Werror \
  -o build/gta3-test/main.legacy.scm
md5sum build/gta3-test/main.legacy.scm
# 694cdeb27de4aee3d20f223604ea5375

# GTA Vice City
legacy_gta3sc build/gtavc-test/main.sc --config=gtavc -pedantic-errors -Werror \
  -Wno-expect-var -frelax-not -o build/gtavc-test/main.legacy.scm
md5sum build/gtavc-test/main.legacy.scm
# 2a7c30f7bdd04a93213c2eaa8103bd72
```

Rewrite driver: `cmake --build build --target gta3sc-cli` then
`./build/src/gta3sc/gta3sc`. Build **`gta3sc-cli`**, not `gta3sc` (library only)
— otherwise the executable may be stale after `lib/` edits.

---

## Two-track compare (header vs body)

Legacy **decompile** only walks the **main segment forward** — it does **not**
surface header slabs (global-var fill, used-object table, mission offset table).

| Region | Tool |
|--------|------|
| **Headers** (global vars, used objects, mission table) | Binary parse both `.scm` files using the `MultifileCodeGen` layout (`0x0002` chain in `lib/codegen/trilogy/multifile-codegen.cpp`). Compare sizes, counts, mission offsets. |
| **Main segment + missions** (instruction stream) | Legacy IR2 decompile + text diff |

If IR2 matches but MD5 differs, check headers first.

---

## IR2 decompile workflow (main segment)

Legacy disassembler (requires `-emit-ir2`; `-o` alone is not enough):

```bash
legacy_gta3sc decompile rewrite.scm -o rewrite.ir2 --config=gta3 -emit-ir2
legacy_gta3sc decompile legacy.scm -o legacy.ir2 --config=gta3 -emit-ir2
diff -u legacy.ir2 rewrite.ir2 | less
```

Use `--config=gtavc` for Vice City fixtures.

IR2 is Sanny-Builder-style explicit IR (~90k lines for full `gta3_main`). Good
properties:

- Human-readable commands (`WAIT 0i8`, `SCRIPT_NAME 'MAIN'`, …)
- Labels and control flow reconstructed
- `diff` / `vimdiff` gives **first semantic divergence** without hand-decoding
  datatype bytes

**Limitations** (per legacy behaviour):

- **No header fields** — global count, used-object list, or mission offset table
  bugs may not appear in IR2 while MD5 still differs.
- Decompiler starts at main segment; early `#DEFINE_MODEL` lines come from used-
  object metadata, not VAR declarations.
- Mission scripts appear as the decompiler walks offsets; compare mission
  boundaries via labels / `@@file` markers in IR2 if present.

Optional: `gta3sc/utils/ir2_to_gta3.py` in the old tree converts IR2 toward
`.sc` syntax for easier reading.

---

## Fallback: raw binary

If IR2 diff is noisy or sync is lost:

1. Parse headers → compute `header_size` and `main_segment_size` on both files.
2. `cmp -l` on `[header_size .. header_size + main_segment_size)` only.
3. Walk opcodes at first mismatch using `CodeEmitter` datatype rules
   (`lib/codegen/trilogy/emitter.cpp`) + command table from config.

For very early mismatches (e.g. byte 8), compare from file start — likely header
layout, not main-segment IR.

---

## Map divergence → compiler phase

| IR2 / binary symptom | Likely phase |
|----------------------|--------------|
| Header size / `#DEFINE_MODEL` count or order | Storage / used-object insertion, header codegen |
| Mission table / offset list wrong | Multifile classification, relocation, mission lowering |
| Same command, different arg (`0i8` vs `0i16`, wrong `&var`) | Emitter int width, var slot assignment |
| Missing/extra commands | Lowering (IF/WHILE, `MISSION_END`, stats, `LOAD_AND_LAUNCH_MISSION`) |
| Divergence only after `@@somefile` label | That subscript/mission file’s codegen |
| Wrong constant value in `SET_*_TO_CONSTANT` | Command-table constant lookup / enum precedence |

After identifying the first hunk, reduce to a minimal `.sc` repro and add a unit
test where practical (see issue sections in `brute-force-summary.md`).
