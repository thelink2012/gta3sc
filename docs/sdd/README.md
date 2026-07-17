# Spec-Driven Development (SDD)

Agent-oriented process for **gta3sc**: write a spec → human reviews → agent implements.
Independent subtasks of one spec can run **concurrently in separate worktrees**.

| Document | Purpose |
|----------|---------|
| **This README** | Philosophy, status lifecycle, agent quickstart |
| [**WORKFLOW.md**](WORKFLOW.md) | End-to-end flow, review gate, worktrees/branches, file layout |
| [**00000000-template/**](00000000-template/) | Copy-paste starting points for specs and subtasks |

Specs live under `docs/sdd/yyyymmdd-slug/` (e.g. `docs/sdd/20260717-compiler-driver/`).
Framework templates live in `docs/sdd/00000000-template/` — they use the same `yyyymmdd-slug` shape so they sort first and are **not** product specs.

Every spec directory has a **`README.md` entrypoint** (small specs: full body in that file; larger specs: index plus stage files). See [WORKFLOW.md](WORKFLOW.md#one-file-vs-many-files).

---

## Core principles

1. **Source code is the source of truth.** Specs are disposable scaffolding for agents and reviewers. Humans may delete a spec directory at any time after the work lands (or even if it never ships). Do not treat missing or deleted specs as a problem.
2. **Human review before implementation.** Agents must not implement from a spec whose status is still `draft` or `review` unless the user explicitly overrides that gate in the current conversation.
3. **Requirements before code shape.** Capture problem, requirements, and design decisions before locking an implementation plan. If coding discovers a new user-visible requirement, update the requirements (and design if needed) before treating it as settled.
4. **Subtasks enable concurrency.** Split independent work into subtasks; each may use its own worktree and `gta3sc-rewrite-branches/...` feature branch.
5. **Stay lean.** Prefer a short accurate spec over bureaucracy. Skip sections that do not apply; mark unknowns as TBD.

Also follow repo agent rules in [`AGENTS.md`](../../AGENTS.md) and design goals in [`DESIGN.adoc`](../../DESIGN.adoc).

---

## Status lifecycle

Every spec (and optionally each subtask) has a `status` in frontmatter:

| Status | Meaning | Agents may implement? |
|--------|---------|------------------------|
| `draft` | Being written; incomplete or unreviewed | **No** |
| `review` | Author believes it is ready; waiting on human | **No** |
| `approved` | Human signed off; ready to implement | **Yes** |
| `in_progress` | Implementation underway | **Yes** (assigned scope only) |
| `done` | Accepted; may be deleted anytime | N/A (no new work) |
| `cancelled` | Abandoned; keep only if useful as history | **No** |

**Review gate:** move `draft` → `review` when ready for the human. Only the human (or an explicit user instruction in-chat) promotes to `approved`. Agents set `in_progress` when they start, and `done` when acceptance criteria pass and the user accepts the result (or when asked to mark done).

---

## Agent quickstart

### Writing a new spec

1. Read [WORKFLOW.md](WORKFLOW.md).
2. Create `docs/sdd/yyyymmdd-slug/` using today's date and a short kebab slug.
3. Small scope → copy [00000000-template/README.single.md](00000000-template/README.single.md) to `README.md`.
4. Larger scope → copy [00000000-template/README.multi.md](00000000-template/README.multi.md) to `README.md`, then add stage files as needed.
5. Fill **Context → Requirements → Design → Plan**; leave implementation detail sparse until requirements stabilize.
6. Set `status: review` and stop for human feedback (interactive sessions: ask).

### Implementing an approved spec

1. Confirm `status` is `approved` or `in_progress` (or user said to implement anyway).
2. Read the spec `README.md`, then follow its document order.
3. If subtasks exist, pick **one** assigned subtask (or the next unblocked one). Prefer a dedicated worktree + feature branch (see WORKFLOW).
4. Implement per [`AGENTS.md`](../../AGENTS.md) (interface → tests → implementation for new APIs).
5. Satisfy **Acceptance**; update the spec only if it helps the next reader—or leave it stale knowing the human may delete it.

### Concurrent subtasks

Independent subtasks listed under Plan (or as files in `subtasks/`) may proceed in parallel worktrees. Do not start a subtask that depends on another that is not yet merged unless the dependency is explicitly waived.

---

## Directory layout (summary)

```text
docs/sdd/
  README.md                 # This file
  WORKFLOW.md
  00000000-template/        # Framework templates (not specs)
  yyyymmdd-slug/            # One directory per spec
    README.md               # Always the entrypoint
    01-context.md           # Optional multi-file stages
    …
    subtasks/               # Optional concurrent units
      01-short-name.md
```

Details, naming, and branch/worktree conventions: [WORKFLOW.md](WORKFLOW.md).
