# SDD workflow

How to write, review, and implement specs in this repo. Read `[README.md](README.md)` for principles and status meanings.

---

## End-to-end flow

```text
vague idea
    │
    ▼
clarify (chat / notes) ──► write spec (status: draft)
    │
    ▼
ready for human ─────────► status: review
    │
    ▼
human feedback ──────────► revise, or status: approved
    │
    ▼
split into subtasks (optional) ──► each: worktree + feature branch
    │
    ▼
implement (status: in_progress) ──► tests / acceptance
    │
    ▼
status: done ──► human may delete the spec dir anytime
```

**Ordered content (dependency order):**


| Stage               | Focus            | Typical content                                                                    |
| ------------------- | ---------------- | ---------------------------------------------------------------------------------- |
| **1. Context**      | Why / for whom   | Problem, goals, **non-goals**, constraints, glossary, brownfield touchpoints       |
| **2. Requirements** | What outwardly   | Observable behaviour, scenarios, errors, config *meanings*, language/CLI semantics |
| **3. Design**       | Decisions        | Chosen approach, alternatives rejected, invariants, interfaces at a high level     |
| **4. Plan**         | How in this repo | Modules/paths, subtasks, order, sharp edges; link to `AGENTS.md` conventions       |
| **5. Acceptance**   | Done when        | Commands, tests, manual checks that must pass                                      |


Later stages may reference earlier ones; earlier stages must not depend on Plan detail. If Plan or coding invents a new outward requirement, amend **Requirements** (and Design if needed) first.

For a pure internal refactor with no user-visible change, Context + Design + Acceptance may suffice; still document invariants.

---

## Naming


| Item             | Convention                                                    | Example                                                        |
| ---------------- | ------------------------------------------------------------- | -------------------------------------------------------------- |
| Spec directory   | `yyyymmdd-slug` under `docs/sdd/`                             | `docs/sdd/20260717-compiler-driver/`                           |
| Slug             | lowercase kebab-case; short                                   | `compiler-driver`, `trilogy-emitter-fix`                       |
| Date             | Spec creation date (UTC or local; be consistent within a day) | `20260717`                                                     |
| Feature branch   | `gta3sc-rewrite-branches/<spec-slug>[/<subtask-slug>]`        | `gta3sc-rewrite-branches/20260717-compiler-driver/cli-options` |
| Integration base | `gta3sc-rewrite` (never `master`)                             | —                                                              |


Use the **creation** date in the directory name even if work spans days. Do not renumber dirs to “look sequential.”

Templates live in `docs/sdd/00000000-template/` so they sort before real dated specs.

---

## One file vs many files

**Every spec directory has `README.md` as the entrypoint.** Listing the directory always shows where to start. Choose size by *what goes in that README*, not by inventing a second entry filename (`SPEC.md` is not used).


| Situation                                                       | Use                                                                                                                               |
| --------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------- |
| Narrow change, spike, single concern                            | Copy [00000000-template/README.single.md](00000000-template/README.single.md) → `README.md` (full Context…Acceptance in one file) |
| Feature needs separate review of requirements vs design vs plan | Copy [00000000-template/README.multi.md](00000000-template/README.multi.md) → `README.md` (index) + stage files                   |
| Independent implementable chunks (esp. parallel agents)         | Add `subtasks/` with [00000000-template/SUBTASK.md](00000000-template/SUBTASK.md)                                                 |


### Recommended multi-file layout

```text
docs/sdd/yyyymmdd-slug/
  README.md                 # Required index (README.multi template)
  01-context.md
  02-requirements.md
  03-design.md
  04-plan.md                # May list subtasks inline and/or point to subtasks/
  subtasks/
    01-cli-options.md
    02-pipeline-wire.md
  reference-foo.md          # Optional descriptive refs (not a fake “stage 5”)
```

**Reading order** is whatever the README table says—not filename sort alone. Prefer splitting a heavy stage (`02-requirements-errors.md`) over inventing a long `05-`/`06-` tail.

Frontmatter on every doc (including indexes):

```yaml
inherits: docs/sdd
spec_id: yyyymmdd-slug
title: …
status: draft
doc_role: …   # e.g. slice-index | single-file | stage-context | subtask
```

---

## Human review gate

Agents **must** check before writing production code from a spec:

1. Spec (and subtask, if any) `status` is `approved` or `in_progress`, **or**
2. The user in this conversation explicitly says to implement despite `draft`/`review`.

If status is `draft` or `review` and there is no override: summarize readiness, set or keep `review` if appropriate, and **stop** for human input.

Humans promote `review` → `approved`. Agents should not self-approve.

---

## Subtasks, worktrees, and branches

Use subtasks when pieces are **independently mergeable** (or clearly sequenced) and you want parallel agents.

### Mapping


| Concept        | Location                                                            |
| -------------- | ------------------------------------------------------------------- |
| Spec           | `docs/sdd/yyyymmdd-slug/`                                           |
| Subtask doc    | `…/subtasks/NN-short-name.md` (or a Plan checklist for tiny splits) |
| Feature branch | `gta3sc-rewrite-branches/yyyymmdd-slug/short-name`                  |
| Worktree       | Prefer the **tool default**; one worktree per active subtask agent  |


### Worktree location

**Prefer the agent/IDE default** when the tool creates worktrees for you. Do not invent a parallel path that fights the tool.


| How worktrees are created                     | Where to put them                                              |
| --------------------------------------------- | -------------------------------------------------------------- |
| Cursor Parallel Agents / Best-of-N            | Tool default (typically `~/.cursor/worktrees/…`) — leave as-is |
| OpenCode or other agents that spawn worktrees | That tool’s documented default                                 |
| Manual `git worktree add` (CLI / scripts)     | Repo-local `**.worktrees/<name>/`** (gitignored)               |


Manual example:

```bash
git fetch origin
git worktree add -b gta3sc-rewrite-branches/20260717-compiler-driver/cli-options \
  .worktrees/cli-options \
  origin/gta3sc-rewrite
```

`.worktrees/` is listed in `.gitignore`. Feature branch naming (`gta3sc-rewrite-branches/...`) is unchanged regardless of worktree path.

### Rules for concurrent agents

- Implement **only** the assigned subtask; do not “helpfully” expand scope into sibling subtasks.
- Respect **Depends on** in the subtask doc; if blocked, stop and report.
- Prefer merging to `gta3sc-rewrite` (or the agreed integration branch) via normal PR flow; resolve conflicts against the integration base, not against `master`.
- Spec markdown may live on the integration branch; subtask agents need not update the parent Plan status unless asked—avoid noisy cross-worktree doc churn. Updating the **subtask** status/`done` criteria in that subtask file is enough when convenient.
- After merge, humans may delete the whole spec directory; that is expected.

### When not to split

Do not create subtasks for tightly coupled edits that will thrash the same files. Prefer one agent / one branch.

---

## Starting from a vague goal

Talk first: goals, non-goals, scenarios, constraints. Then pour stable answers into Context and Requirements. Scratch notes are fine until the structure settles.

Optional discovery habit: ask clarifying questions in short rounds; do not create files until the user wants them (interactive mode—see `AGENTS.md`).

---

## After shipping / drift

Code and tests win. Options:

1. Update the spec to match reality, or
2. Leave it; delete the directory when no longer useful, or
3. Add a one-line **Drift** note only if someone will keep reading the spec shortly.

Never block merges on keeping prose perfectly synced. Specs are aids, not contracts after `done`.

---

## Relation to existing repo docs


| Doc                                | Role vs SDD                                                                      |
| ---------------------------------- | -------------------------------------------------------------------------------- |
| `[AGENTS.md](../../AGENTS.md)`     | Always-on coding/build/test conventions; SDD does not replace it                 |
| `[DESIGN.adoc](../../DESIGN.adoc)` | Long-lived architecture; specs should not contradict it without calling that out |
| `docs/sdd/00000000-template/`      | Process templates only                                                           |


