# Brute-force integration issues

Problem specs found while compiling real main-script fixtures through the rewrite
pipeline. Each section: symptom, minimal repro, acceptance criterion.

| Game | Fixture | Legacy test |
|------|---------|-------------|
| GTA III | `build/gta3-test/main.sc` + `main/*.sc` | `gta3.test` |
| GTA Vice City | `build/gtavc-test/main.sc` + `main/*.sc` | `gtavc.test` |

Bytecode diff workflow: [`bytecode-diff-methodology.md`](bytecode-diff-methodology.md).

---

## Integration golden — GTA III

**RED:** compile fails (parse / sema / codegen) or output MD5 ≠ legacy.

**Repro:**

```bash
cmake --build build --target gta3sc-cli
./build/src/gta3sc/gta3sc
md5sum build/gta3-test/main.scm
```

**Accept:** MD5 `694cdeb27de4aee3d20f223604ea5375` (606553 bytes), byte-identical to:

```bash
legacy_gta3sc build/gta3-test/main.sc --config=gta3 -pedantic-errors -Werror \
  -o build/gta3-test/main.legacy.scm
```

---

## Integration golden — GTA Vice City

**RED:** compile fails or output MD5 ≠ legacy.

**Repro:**

```bash
cmake --build build --target gta3sc-cli
./build/src/gta3sc/gta3sc
md5sum build/gtavc-test/main.scm
```

**Accept:** MD5 `2a7c30f7bdd04a93213c2eaa8103bd72` (1269133 bytes), byte-identical to:

```bash
legacy_gta3sc build/gtavc-test/main.sc --config=gtavc -pedantic-errors -Werror \
  -Wno-expect-var -frelax-not -o build/gtavc-test/main.legacy.scm
```

**Fixture notes:** decompiler-style sources (flat `ANDOR` / `GOTO_IF_FALSE` /
`NOT cmd` / relationals, lowercase `var_*` / `timera`). Legacy requires
`-frelax-not`. Level models need `data/gta_vc.dat` (31 map IDEs), not
`data/default.dat` — see issue 9b.

---

## 1. CRLF word-token boundaries (scanner)

**Phase:** preprocessor → scanner → parser  
**Symptom:** `error: Expected argument` on compound assignment; lexeme includes `\r`.

**Repro** (CRLF line endings required):

```sc
SCRIPT_NAME MAIN\r
$g2 = 0.0625\r
$g3 = $g2\r
$g3 /= 2.0\r
```

Fixture: `main.sc` ~435–438.

**Accept:** parses; `$g3 /= 2.0` is valid compound assignment.

**Tests:** `scanner word spelling before CRLF`, `parsing expressions with CRLF line endings`.

---

## 2. Subscript file shape — flat `MISSION_START` … EOF (parser)

**Phase:** parser (multifile subscripts)  
**Symptom:** Subscript parse fails or mis-splits body when real missions deviate
from `MISSION_START … body … MISSION_END … trailing code`.

**Repro — `intro.sc`:**

```sc
MISSION_START
//GOSUB mission_start_intro
//MISSION_END


VAR_INT cs_cathead cs_robb ...
mission_start_intro:
    ...
```

**Repro — `hj.sc`:** `MISSION_END` inside `{ }` with code after.

**Accept:** `parse_subscript_file()` succeeds on both (CRLF fixtures under
`build/gta3-test/main/`).

**Tests:** `parsing intro subscript file`, `parsing hj subscript file`.

---

## 3. Variable name vs string constant (sema)

**Phase:** sema (discover pass)  
**Symptom:** `Variable name already used as string constant` on valid scripts.

**Repro** (needs `RED`/`GREEN` as `BLIPCOLOUR` in config, as in GTA3 `constants.xml`):

```sc
VAR_INT red green
```

Fixture: `main/ray3.sc`.

**Accept:** sema passes; `red` and `green` are variables.

**Test to add:** sema fixture with `VAR_INT red` + `BLIPCOLOUR` constants.

---

## 4. `START_NEW_SCRIPT` — global label before `{` block (sema)

**Phase:** sema  
**Symptom:** ~142× `Target label not within scope` on `main.sc` thread spawns.

**Repro:**

```sc
entry:
{
    WAIT 0
}
START_NEW_SCRIPT entry
```

Fixture pattern: `hood_mission1_loop:` then `{ ... }`, then
`START_NEW_SCRIPT hood_mission1_loop` elsewhere in `main.sc`.

**Accept:** sema passes; zero-arg `START_NEW_SCRIPT` resolves the global label to
the scope opened by the `{` after the label.

**Tests:** `sema hardcoded START_NEW_SCRIPT` → subcases for global thread label
(with and without argument passing).

---

## 5. `DEFAULTMODEL` / level models (config + sema)

**Phase:** config loading + sema  
**Symptom:** Undefined enum / model errors for short IDE names (`MAFIA`, `BFINJECT`, …)
used as `DEFAULTMODEL` or car-generator model args.

**Repro:**

```sc
CREATE_CAR_GENERATOR ... MAFIA ... gen_id
```

**Constraints observed:**

| Approach | Problem |
|----------|---------|
| `constants.xml` only | Partial `DEFAULTMODEL` set (`CAR_MAFIA` style, not `MAFIA`) |
| `load_models_from_level(..., objs_only=false)` | ~870× `Invalid IDE line` — car CSV lines not parsed |
| Merge `gta3/default.xml` | Workaround only — full constant table |

**Unit repro:**

```sc
VAR_INT x
CREATE_CAR LEVEL_MODEL 0.0 0.0 0.0 x
```

With `LEVEL_MODEL` in `ModelTable` from IDE.

**Accept (unit):** sema passes — test `sema used objects` →
`DEFAULTMODEL params resolve level models`.

**Accept (integration):** full `gta3_main` compiles without `default.xml` merge
once cars IDE parsing works or constants + IDE objs cover all names used.

**Note:** vars like `joeys_buggy` are declared in `main/car_gen.sc` line 12 —
not implicit globals.

---

## 6. Nested `IF` — missing `GOTO_IF_FALSE` (lowering)

**Phase:** lowering (`IfStmtRewriter`)  
**Symptom:** MD5 mismatch; IR2 first semantic diff at `MainLoop` (~line 1087):
legacy branches between nested guards; rewrite chains `ANDOR` without
`GOTO_IF_FALSE`.

**Repro** (same shape as `main.sc` 1330–1357):

```sc
MainLoop:
WAIT 1000
IF IS_PLAYER_PLAYING player
    IF IS_COLLISION_IN_MEMORY 1
        WAIT 0
    ENDIF
ENDIF
GOTO MainLoop
```

**Accept:** IR2 around `MainLoop` matches legacy (two `GOTO_IF_FALSE` before
pickup logic); full fixture MD5 matches golden.

**Tests:** `nested IF` → `nested IF directly after condition`;
`unittest/syntax/lowering/if-stmt-rewriter.cpp`.

**Note:** First binary diff at byte 36478 (`GOSUB_FILE` fixup) was a cascade from
this bug, not a separate multifile ordering issue.

---

## 7. CMake target naming (infra)

**Symptom:** Library rebuild appears to have no effect; same `.scm` MD5 after edits
under `lib/`.

**Repro:** `cmake --build build --target gta3sc` after editing `lib/` — `libgta3sc.a`
updates but `./build/src/gta3sc/gta3sc` timestamp unchanged.

**Accept:** `cmake --build build --target gta3sc-cli` relinks the driver after lib
changes.

| CMake target | Builds |
|--------------|--------|
| `gta3sc` | Static library `libgta3sc.a` |
| `gta3sc-cli` | Executable `src/gta3sc/gta3sc` |

---

## 9. Statement-level `NOT` + relational (parser) — fixture artifact

**Phase:** parser  
**Not R\* language.** The `gtavc-test` sources are decompiler round-trip output,
not original Rockstar scripts. Lines like `NOT timera > 1000` are a flat
control-flow artifact of that pipeline — not valid GTA3Script as authors would
write it. Legacy accepts them only with `-frelax-not` (same hack the integration
test uses). Parity work must handle this pattern; do not treat it as a general
language requirement.

**Symptom:** `error: Expected argument` at `>` in `NOT timera > 1000`.

**Repro** (decompiler-style flat control flow in `gtavc-test` only):

```sc
ANDOR 0
NOT timera > 1000
GOTO_IF_FALSE main_13
```

Fixtures: `main.sc` ~9092, `main/lawyer2.sc`, etc.

**Accept:** parses as negated relational statement when `-frelax-not` semantics
apply (legacy / `gtavc.test` parity).

**Tests:** update `parsing AND/OR/NOT outside of condition` (currently expects
failure).

---

## 9b. Level IDE loading — `gta_vc.dat` (config + driver)

**Phase:** config / driver  
**Symptom:** ~3550× `Undefined variable` on IDE model names (`DTN_STADDOORA`,
`BRIBE`, …) when level dat is `default.dat`.

**Cause:** VC `default.dat` references only `IDE DATA\DEFAULT.IDE`. Map object
names live in 31 IDEs listed from `gta_vc.dat`.

**Accept:** sema resolves `CREATE_OBJECT` / `CREATE_PICKUP` model args when driver
loads `gta_vc.dat` with `objs_only=true`.

---

## 10. `peek_expression_type` — leading whitespace (parser)

**Phase:** parser  
**Symptom:** Indented relational lines parsed as commands (`timera` command, `>`
fails in `parse_argument()`).

**Repro:**

```sc
    timera > 1000
```

**Accept:** recognized as conditional expression statement.

---

## 11. SCM global-var chunk header marker (codegen)

**Phase:** multifile codegen  
**Symptom:** First binary diff at byte 8: legacy `0x6D`, rewrite `0x00`.

**Repro:** compile any multifile script; compare byte 8 of output SCM to legacy.

**Accept:** byte 8 matches legacy (`0x6D`).

**Test:** `global var chunk header bytes`.

---

## 12. Float literal encoding (codegen)

**Phase:** codegen  
**Symptom:** Wrong float payload size or value in SCM; MD5 mismatch on fixtures
using float literals. VC expects 4-byte IEEE floats, not GTA3-style Q11.4.

**Repro:** compile fixture with float literals (e.g. `$g2 = 0.0625` in `main.sc`);
IR2 or binary diff shows wrong datatype / size at first float constant.

**Accept:** float constants match legacy emitter layout for the target game.

**Tests:** `unittest/codegen/trilogy/emitter.cpp`, `codegen.cpp`.

---

## 13. Ambiguous constant names — `SNIPER` (command table)

**Phase:** command table / sema  
**Symptom:** IR2 diff at `main/rampage.sc` ~452 (`var_1518 = SNIPER`):

```diff
-SET_VAR_INT_TO_CONSTANT &6072 285i16
+SET_VAR_INT_TO_CONSTANT &6072 7i8
```

**Cause:** `SNIPER` exists in **CAMMODE** (7) and **DEFAULTMODEL** (285).
Legacy prefers **DEFAULTMODEL** ([gta3sc#60](https://github.com/thelink2012/gta3sc/issues/60)).

**Accept:** `find_constant_any_means("SNIPER")` → 285; full VC fixture MD5 matches.

**Test:** `find_constant_any_means prefers DEFAULTMODEL`.
