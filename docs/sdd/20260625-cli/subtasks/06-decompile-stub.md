---
inherits: docs/sdd
spec_id: 20260625-cli
title: Subtask — decompile stub
status: approved
doc_role: subtask
subtask_id: 06-decompile-stub
---

# Subtask: decompile stub

Parent spec: [`../README.md`](../README.md).

**Status:** approved — unblocked (stub only)

**Source code is truth.** This subtask doc may be deleted with the parent spec.

---

## Intent

Route `gta3sc decompile …` to a dedicated runner that hard-errors with a clear
“decompiler not yet implemented” message (not a silent/generic stub forever).

## Scope

- In: `run_decompile` + parse stub as needed, wire `Action::decompile` in
  `run.cpp`, unit test for exit code + message.
- **Out:** actual decompiler, `-emit-ir2`, recursive traversal, lit decompile
  matrix.

## Depends on

- None for the stub.
- Soft: coordinate `run.cpp` with subtask `05-query-config-path`.
- Full decompile remains blocked on a decompiler backend (separate work).

## Design notes (delta only)

- Parent Plan Phase 6: stub error string; honour-or-error for unwired flags
  still applies if any flags are parsed later.

## Implementation sketch

- Touch points: `lib/cli/run-decompile.cpp`, header, `run.cpp`, CMake,
  `unittest/cli/…`.
- Order: interface → test expecting message → dispatch.

## Branch / worktree

- Branch: `gta3sc-rewrite-branches/20260625-cli/decompile-stub`
- Worktree: prefer tool default; manual → `.worktrees/decompile-stub/`

## Acceptance

- [ ] `decompile` subcommand returns failure with the agreed not-implemented
      message.
- [ ] Does not claim success or write output.
- [ ] Full `gta3sc_unittest` green before done.

## Notes / TBD

- Parallel with Phase 4 storage work; only conflict surface is `run.cpp`.
