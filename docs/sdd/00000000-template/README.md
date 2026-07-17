# SDD templates (`00000000-template`)

Copy these into a new `docs/sdd/yyyymmdd-slug/` directory. Do not edit templates in place for a feature—copy first.

The directory name `00000000-template` uses the same `yyyymmdd-slug` shape as real specs so it sorts first under `docs/sdd/` and is obviously not a dated product spec.

## Entrypoint convention

**Every spec directory must have `README.md` as the entrypoint.** That way a directory listing always shows where to start. There is no separate `SPEC.md` filename for product specs.

| Template | Copy to | When to use |
|----------|---------|-------------|
| [**README.single.md**](README.single.md) | `README.md` | Single-file spec (small / narrow)—full Context…Acceptance in one file |
| [**README.multi.md**](README.multi.md) | `README.md` | Index for a multi-file spec folder |
| [**SUBTASK.md**](SUBTASK.md) | `subtasks/NN-short-name.md` | One independently implementable unit (often one worktree) |

Stage file names for multi-file specs (create as needed; empty stages may be omitted):

- `01-context.md`
- `02-requirements.md`
- `03-design.md`
- `04-plan.md`

Put descriptive references (API notes, opcode tables, etc.) beside them with clear names; index them in the README.

See [../WORKFLOW.md](../WORKFLOW.md) for status, review gate, and branch/worktree naming.
