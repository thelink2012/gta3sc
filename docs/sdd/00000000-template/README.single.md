---
inherits: docs/sdd
spec_id: yyyymmdd-slug
title: Concise title
status: draft
doc_role: single-file
---

<!--
  Small / narrow specs: copy this file to README.md in the new spec dir.
  Larger work: use README.multi.md as README.md plus 01–04 stage files
  and optional subtasks/ (see docs/sdd/WORKFLOW.md).
  Always use README.md as the entrypoint—do not invent SPEC.md.
-->

# Spec: <title>

**Source code is the source of truth.** This file is disposable scaffolding. Prefer merged code/tests on conflict; humans may delete this directory anytime.

**Status:** draft | review | approved | in_progress | done | cancelled

---

## 1. Context

Why this exists and what success means—without locking file layout.

- Problem: …
- Goals: …
- **Non-goals:** …
- Constraints: … (language version, compatibility, performance, `DESIGN.adoc` invariants, …)
- Brownfield: existing modules/behaviour we are **not** re-specifying unless they change: …
- Glossary (optional): …

---

## 2. Requirements

Observable behaviour (compiler output, diagnostics, CLI UX, API contracts)—not yet “we will add `foo.cpp`.”

- Normal paths: …
- Errors / diagnostics: …
- Configuration / flags **as concepts** (semantics first; concrete flag names OK when they *are* the requirement): …
- Compatibility / versioning notes: …

---

## 3. Design

Decisions that constrain implementation. Keep alternatives brief.

- Approach: …
- Alternatives considered: … → rejected because …
- Key invariants / interfaces: …
- Open questions: …

---

## 4. Plan

How this repo realizes the requirements (touch points, order, subtasks).

- Touch points: `include/…`, `lib/…`, `unittest/…`, …
- Implementation order: …
- Subtasks (optional; link to `subtasks/*.md` or list here):
  - [ ] `short-name` — … (branch: `gta3sc-rewrite-branches/<spec-id>/short-name`)
- Out of scope follow-ups: …

---

## 5. Acceptance

Authoritative checks for “done”:

- Build / tests: e.g. `./build/unittest/gta3sc_unittest --test-case="…"`
- Behaviour checks: …
- Docs / AGENTS updates (only if required): …
