# Compilation driver tests — remaining work

Open items for `unittest/driver/compilation.cpp` and
`gta3sc::driver::Compilation`.

## Planned coverage

### Lowering pass driver tests

Done. Added six top-of-funnel compilation tests in `compilation.cpp`:

- `compile - pre-sema lowering - REPEAT stmt lowered`
- `compile - post-sema lowering - IF stmt lowered`
- `compile - post-sema lowering - WHILE stmt lowered`
- `compile - post-sema lowering - scope blocks removed`
- `compile - post-sema lowering - var decls removed`
- `compile - post-sema lowering - stats arg rewritten`

`MissionStmtRewriter` and `LoadAndLaunchMissionRewriter` remain covered by the
existing multifile test.

Also: each lowering step in `lib/driver/compilation.cpp` now owns its own
`NameGenerator` with a descriptive prefix (`REPEAT_`, `IF_`, `WHILE_`) instead
of sharing a single `LOWER_` instance. The `CommandTableFixture` was extended
with opcode IDs for `ANDOR`, `GOTO_IF_FALSE`, `GOTO_IF_TRUE`, `SET_VAR_INT`,
and `SET_COLLECTABLE1_TOTAL`, plus the `ADD_THING_TO_THING` /
`IS_THING_GREATER_OR_EQUAL_TO_THING` alternators (with `ADD_INT_TO_VAR_INT` /
`IS_INT_VAR_GREATER_OR_EQUAL_TO_INT` implementations) required to compile a
`REPEAT` loop end-to-end.

---

## Related driver backlog (not only this test file)

In `lib/driver/compilation.cpp`, `codegen()` stops on the first failing step.
There is a TODO to explore running storage allocation, generation, and
relocation without early exit so all error diagnostics are collected in one
run.
