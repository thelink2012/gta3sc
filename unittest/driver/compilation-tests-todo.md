# Compilation driver tests — remaining work

This note tracks open items for `unittest/driver/compilation.cpp` and the
`gta3sc::driver::Compilation` pipeline those tests exercise.

## Failing until product or fixture catches up

### Multifile happy path

**Test:** `compile - multifile all file types succeeds`

`sub.sc` / `miss.sc` use `MISSION_START` / `MISSION_END`, which are intentionally
**not** in `CommandTableFixture` (they are not normal commands). Semantic
analysis currently reports **undefined command** for those tokens until a
**lowering pass** (or equivalent) strips or rewrites them before sema/codegen.

### Relocation failure

**Test:** `compile - codegen phase failure - relocation failure`

Same root cause: the mission shell uses `MISSION_START` / `MISSION_END`, so the
run never reaches the **`label_ref_across_segments`** path. You get
**`undefined_command`** (and extra diagnostics) instead, so assertions and the
diagnostic fixture do not match the intended scenario.

---

## `may_fail` — remove after fixes

### Local storage overflow

**Test:** `compile - codegen phase failure - local storage overflow`
(`doctest::may_fail`)

**Issue:** `StorageTable::from_symbols()` (or the driver) should emit an **error
diagnostic** on overflow. Today **`compile()`** can return **`false`** with **no
diagnostic**, so `CHECK_FALSE(diags.empty())` and unconditional **`consume_diag()`**
are noisy until that is fixed.

**After fix:** add a **`CHECK`** on the real **`DiagnosticDescriptor`**, drop
**`may_fail`**, and optionally guard **`consume_diag()`** if you want cleaner
failure output when the queue is empty.

### Bytecode / codegen vs diagnostics

**Test:** `compile - codegen phase failure - bytecode failure`
(`doctest::may_fail`)

**Issue:** **`Compilation::codegen()`** should not report success when codegen
emits an **error**-severity diagnostic (e.g. **`target_does_not_support_command`**
for `COMMAND_WITHOUT_ID`). Today **`MultifileCodeGen::generate()`** can still
return success; see **`lib/driver/compilation.cpp`** (~line 77):
`// TODO check for diagman errors before returning true?`

**After fix:** remove **`may_fail`** and keep **`CHECK_FALSE(result)`**.

---

## Planned coverage

### Lowering pass

The file notes a **TODO** to add driver tests once the **lowering pass** exists.
Revisit multifile and mission-shell scenarios when lowering defines how
`MISSION_START` / `MISSION_END` (and related constructs) interact with sema and
codegen.

---

## Related driver backlog (not only this test file)

In **`lib/driver/compilation.cpp`**, other TODOs (lowering, relocation staged
differently, output interface, `AbstractCompilation`) are not strictly required
to finish this test file, but they are natural places for work that unlocks or
simplifies the cases above.

---

## Summary

Unblockers for a clean, fully passing **`compilation.cpp`** suite:

1. **Mission script surface:** lowering (or test data that does not rely on
   treating `MISSION_*` as table commands) so multifile and relocation tests
   reach the intended paths.
2. **Storage overflow:** emit a proper error diagnostic (and align **`compile()`**
   with it).
3. **Codegen + diagnostics:** **`compile()`** / **`codegen()`** must fail when
   error diagnostics were reported, independent of sub-step return values.
