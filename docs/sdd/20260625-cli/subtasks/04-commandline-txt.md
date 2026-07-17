---
inherits: docs/sdd
spec_id: 20260625-cli
title: Subtask — commandline.txt merge
status: approved
doc_role: subtask
subtask_id: 04-commandline-txt
---

# Subtask: commandline.txt merge

Parent spec: [`../README.md`](../README.md).

**Status:** approved — **blocked**

**Source code is truth.** This subtask doc may be deleted with the parent spec.

---

## Intent

When `--config=<name>` is used, whitespace-tokenise that game’s
`commandline.txt` (if present) and merge tokens into the same
`OptionParser` path, with legacy recursion guard.

## Scope

- In: tokeniser (unit-tested), file read via `FilePool`/`SourceManager`,
  merge precedence + recursion guard, compile (and later shared) parse
  re-entry.
- **Out:** inventing new flags; shipping config content (external submodule
  prereq).

## Depends on

- Subtasks `01`–`03` far enough that a real `commandline.txt` only references
  **honoured** flags (else honour-or-error fails by design).
- Config tree ships per-game `commandline.txt` (`BACKLOG.md` / gta3script-config).

## Design notes (delta only)

- Parent Design: `commandline.txt` optional until shipped; `--config` already
  works from `config.xml` alone.
- Same `span<string_view>` parser interface as argv.

## Implementation sketch

- Touch points: new tokeniser in lib, `run-compile` / config bootstrap,
  unittest for tokeniser + merge + recursion.
- Do not start until prereqs above are met.

## Branch / worktree

- Branch: `gta3sc-rewrite-branches/20260625-cli/commandline-txt`
- Worktree: prefer tool default; manual → `.worktrees/commandline-txt/`

## Acceptance

- [ ] Tokeniser + recursion guard unit-tested.
- [ ] Merge precedence matches legacy intent documented in parent Design/Plan.
- [ ] Full `gta3sc_unittest` green before done.

## Notes / TBD

- **Do not implement yet** — blocked on flag surface + config content.
