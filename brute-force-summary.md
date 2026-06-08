# Brute-force integration issues

Problem specs found while compiling real main-script fixtures through the rewrite
pipeline. Each section: symptom, minimal repro, acceptance criterion.

| Game | Fixture | Legacy test | Status |
|------|---------|-------------|--------|
| GTA III | `build/gta3-test/main.sc` + `main/*.sc` | `gta3.test` | GREEN |
| GTA III (R\* source) | `build/undefinified-liberty/main.sc` + `main/` | — | GREEN |
| GTA Vice City | `build/gtavc-test/main.sc` + `main/*.sc` | `gtavc.test` | GREEN |
| GTA Vice City (R\* source) | `build/undefinified-miami/main.sc` + `main/` | — | compiles; MD5 ≠ tarball |

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

## Integration golden — UndefinifiedLiberty (GTA III, R\* source)

**Fixture:** [UndefinifiedLiberty](https://github.com/Sergeanur/UndefinifiedLiberty) →
`build/undefinified-liberty/main.sc` + mission files under `Main/{Commercial,Industrial,Suburban}/`.
Original Rockstar authoring style (`IF`/`WHILE`/`MISSION_START`), not the decompiler
`gta3-test` tarball.

**GREEN:** rewrite compiles and output is byte-identical to legacy.

**Repro:**

```bash
# clone → build/undefinified-liberty; symlink main → Main (issue 15)
cmake --build build --target gta3sc-cli
# driver: gta3 config, gta3/default.xml, data/gta3.dat, TrilogyGame::Gta3
./build/src/gta3sc/gta3sc
md5sum build/undefinified-liberty/main.scm
```

**Accept:** MD5 `db1d01ebf1dda5d978f99c315cbf3775` (606454 bytes), byte-identical to:

```bash
# sync legacy runtime config if needed (issue 18)
cp gta3sc/config/gta3/{constants,commands}.xml gta3sc/build/config/gta3/
cd build/undefinified-liberty
gta3sc/build/gta3sc main.sc --config=gta3 -pedantic-errors -Werror \
  -o main.legacy.scm
cmp main.scm main.legacy.scm
```

**Depends on:** issues 11–12 (`TrilogyGame::Gta3`), 16–17 (gta3script-config), 19
(model/var collision), `Global="true"` enum loading (issue 16 note). Driver must
pass `TrilogyGame::Gta3` to `Compilation`.

**Fixture notes:** overlaps Miami on issue 14 (`WHILE NOT` / `OR NOT` multiline conditions).

---

## Integration golden — UndefinifiedMiami (GTA Vice City, R\* source)

**Fixture:** [UndefinifiedMiami](https://github.com/Sergeanur/UndefinifiedMiami) →
`build/undefinified-miami/main.sc` + `main/*.sc`.

**RED:** compile fails.

**Repro:**

```bash
cmake --build build --target gta3sc-cli
# driver: gtavc config, gtavc/default.xml, data/gta_vc.dat
./build/src/gta3sc/gta3sc
md5sum build/undefinified-miami/main.scm
```

**Accept (rewrite compiles):** MD5 `0b1ced1f7dc66bd22dfd94ac65b8d4cd` (1269133 bytes).

**Fixture notes:** same byte size as `gtavc-test` golden but different MD5 (curated
source vs decompiler tarball). See issue 14.

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
`DEFAULTMODEL params resolve level models` (see also issue 19 for name collisions).

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

**Symptom:** First binary diff at byte 8; MD5 mismatch on multifile fixtures.

**Repro:** compile multifile script; compare byte 8 of output SCM to legacy.

| Game | Legacy byte 8 |
|------|---------------|
| GTA III | `0x00` (`gta3.test`, UndefinifiedLiberty) |
| GTA Vice City | `0x6D` (`gtavc.test`) |

**Accept:** byte 8 matches legacy for the target game.

---

## 12. Float literal encoding (codegen)

**Phase:** codegen  

**Symptom:** Wrong float payload size or value in SCM; MD5 mismatch on fixtures
using float literals.

| Game | Expected encoding |
|------|-------------------|
| GTA III | Q11.4 half-float (2 payload bytes) |
| GTA Vice City / SA | 4-byte IEEE float |

**Repro:** compile fixture with float literals (e.g. `one_sixteenth = 0.0625` in
`main.sc`); IR2 or binary diff shows wrong datatype / size at first float constant.

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

---

## 14. `WHILE NOT` line break before `OR` chain (parser) — R\* source

**Phase:** parser  
**R\* language.** Original mission sources ([UndefinifiedMiami](https://github.com/Sergeanur/UndefinifiedMiami),
[UndefinifiedLiberty](https://github.com/Sergeanur/UndefinifiedLiberty))
use multiline `WHILE NOT` / `OR NOT` condition lists. The `gtavc-test` / `gta3-test`
decompiler tarballs do not contain this authoring style.

**Symptom:** `error: Expected command` at first `OR NOT …` line after a `WHILE NOT`
line with nothing following `NOT`.

**Repro** — `main/range.sc` ~209–217 (UndefinifiedMiami):

```sc
WHILE NOT 
OR NOT HAS_MODEL_LOADED COLT45
OR NOT HAS_MISSION_AUDIO_LOADED 1
OR NOT HAS_MISSION_AUDIO_LOADED 2
OR NOT HAS_MODEL_LOADED faketarget
	WAIT 0
      
ENDWHILE
```

**Related (same file, more typical):** first condition on the `WHILE NOT` line,
`OR NOT` continuations below — e.g. `main/range.sc` ~219–227, `main/taxiwar1.sc`
~56–59, many other missions.

**Open question:** is a lone `NOT` at EOL meaningful, or a line-wrap quirk
equivalent to starting the list on the next `OR NOT` line? Compare bytecode vs
the “typical” form when both appear in one file.

**Accept:** `build/undefinified-miami/main.sc` compiles through this construct.

**Integration repro:**

```bash
cmake --build build --target gta3sc-cli
# driver input: build/undefinified-miami/main.sc
./build/src/gta3sc/gta3sc
```

---

## 15. Subscript directory — `Main` vs `main` (fixture)

**Phase:** driver / `SourceManager`  
**Symptom:** `could not load file` / missing subscripts when repo uses `Main/`
(capital M) but driver scans `main/` (stem of `main.sc`).

**Repro:** [UndefinifiedLiberty](https://github.com/Sergeanur/UndefinifiedLiberty) layout —
missions live under `Main/Commercial/`, `Main/Industrial/`, `Main/Suburban/`.

**Accept:** all `GOSUB_FILE` / `LOAD_AND_LAUNCH_MISSION` targets resolve; compile
proceeds past multifile load. Filename lookup is already case-insensitive; directory
stem must match (`main` → `Main` symlink or rename).

---

## 16. `gta3script-config` constants gap (config)

**Phase:** config (`gta3script-config/config/gta3/constants.xml`)  

**Symptom:** ~143× `Undefined variable` on UndefinifiedLiberty — enum constants that
exist in legacy `gta3sc` config but not in `gta3script-config` (e.g. `PEDGRP_*`,
`GARAGE_HIDEOUT_INDUSTRIAL`, `RADAR_SPRITE_SAVE`, `SOUND_*`, `GANG_TRIAD`).

**Repro:** compile `build/undefinified-liberty/main.sc` against stock
`gta3script-config` before constants sync.

**Accept:** sema resolves symbols used in `main.sc` and mission files; no undefined
constant names that legacy `gta3sc` accepts.

**Note:** rewrite config loader must honor `Global="true"` on enums so names like
`DAY`/`NIGHT` resolve globally.

---

## 17. `SET_GARAGE` garage-type param (config)

**Phase:** config (`gta3script-config/config/gta3/commands.xml`)  

**Symptom:** ~26× `Undefined variable` on `GARAGE_*` tokens in `SET_GARAGE` calls
despite those names being in `constants.xml` `GARAGE` enum.

**Repro** — `main.sc` ~227:

```sc
SET_GARAGE 891.3 -311.1 7.7 898.4 -315.5 12.7 GARAGE_HIDEOUT_INDUSTRIAL save_cars1
```

7th argument is a `GARAGE` enum constant; 8th is output garage var.

**Cause:** `SET_GARAGE` 7th param was plain `INPUT_INT` with no `Enum="GARAGE"`;
sema treated `GARAGE_HIDEOUT_INDUSTRIAL` as a variable name.

**Accept:** `SET_GARAGE` 7th param has `Enum="GARAGE"`; UndefinifiedLiberty sema
passes garage setup block in `main.sc`.

**Note:** same bug in legacy `gta3sc` stock `build/config/gta3/commands.xml`
(issue 18); legacy fails Liberty until config is synced.

---

## 18. Legacy `gta3sc` runtime config stale (infra)

**Phase:** legacy driver config layout  
**Symptom:** legacy `gta3sc` fails UndefinifiedLiberty (~143× `no variable with this
name`) while rewrite passes after gta3script-config fixes — despite legacy source
tree `gta3sc/config/gta3/` having the constants.

**Cause:** legacy binary loads config from `gta3sc/build/config/` (adjacent to the
executable), not the source-tree `gta3sc/config/`. The `build/config` copy can lag
behind source (missing constants, same `SET_GARAGE` gap as issue 17).

**Repro:**

```bash
cd build/undefinified-liberty
gta3sc/build/gta3sc main.sc --config=gta3 -Werror -o main.legacy.scm
# fails on stock build/config; succeeds after syncing from gta3sc/config/gta3/
```

**Accept:** legacy compiles Liberty with config at least as complete as
gta3script-config (issues 16–17).

---

## 19. Model name vs variable name collision (sema)

**Phase:** sema (MODEL / `DEFAULTMODEL` params)  
**Symptom:** IR2 diff on `CREATE_OBJECT` model arg — legacy emits a **global var
reference** (`&184`) where rewrite emits a **used-object negative id** (`-16i8`);
used-object table count/order diverges; MD5 mismatch on UndefinifiedLiberty even
when float encoding and header bytes match.

**Cause:** legacy `ProgramContext::is_model_from_ide` only treats names from
`default.dat` / level `.dat` model tables (plus a small hardcoded fallback) as
“IDE models” for collision resolution. With `-pedantic-errors` (`constant_checks`),
legacy resolves a MODEL param to an existing **variable** when the name is **not**
`is_model_from_ide`, even if the rewrite `ModelTable` (full IDE via `objs_only`)
contains that name. Rewrite initially preferred `ModelTable` whenever
`find_model()` succeeded.

**Repro — var wins (Liberty `main.sc`):**

```sc
VAR_INT plysav_lftdr_lft
CREATE_OBJECT_NO_OFFSET plysav_lftdr_lft 103.85 -482.8 16.25 plysav_lftdr_lft
```

`plysav_lftdr_lft` is in the IDE but not in legacy’s `is_model_from_ide` set → legacy
uses var ref for the model argument; rewrite must not register a used object here.

**Repro — model wins (hardcoded fallback):**

```sc
VAR_INT playersdoor
CREATE_OBJECT_NO_OFFSET playersdoor 890.883 -307.74 8.75 playersdoor
```

`PLAYERSDOOR` is in legacy’s hardcoded `is_model_from_ide` list → used-object
`-1i8` for the model arg even though `playersdoor` is also a `VAR_INT`.

**Resolution order (match legacy pedantic):**

1. `DEFAULTMODEL` enum constant, if any.
2. If a same-named variable exists **and** `!is_model_from_ide(name)` → var ref.
3. Else if `ModelTable` has the name → used object.
4. Else enum / var fallback as today.

**`is_model_from_ide` (rewrite):** `DEFAULTMODEL` constant lookup, then legacy
hardcoded names (`PLAYERSDOOR`, `DEADMAN1`, `BACKDOOR`, `HELIX_BARRIER`,
`AIRPORTDOOR1`, `AIRPORTDOOR2`). Do **not** treat every IDE entry from
`load_models_from_level(..., objs_only=true)` as `is_model_from_ide`.

**Accept:** UndefinifiedLiberty MD5 matches legacy; IR2 `CREATE_OBJECT` model args
and `#DEFINE_MODEL` list match.

**Tests:** `sema used objects` → `variable wins when same name as level-only model`;
`level-only model becomes used object without name collision`.
