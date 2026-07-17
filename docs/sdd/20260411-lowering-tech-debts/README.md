---
inherits: docs/sdd
spec_id: 20260411-lowering-tech-debts
title: Lowering tech debts
status: draft
doc_role: single-file
---

# Spec: Lowering tech debts

**Source code is the source of truth.** This file is disposable scaffolding. Prefer merged code/tests on conflict; humans may delete this directory anytime.

**Status:** draft

---

## 1. Context

Small tech debts in the lowering / `ParserIR` area. No broader feature redesign.

- **Non-goals:** Rewriting lowering passes; changing IR semantics beyond the helpers below.

---

## 2. Requirements

### Label-only `ParserIR` construction

Lowering passes and their unit tests sometimes need a **label-only** instruction (a `ParserIR` with a `LabelDef` and no command). That is expressed today as:

```cpp
ParserIR::create(label, nullptr, allocator)
```

Examples:

- `MissionStmtRewriter::visit_mission_start` in `lib/syntax/lowering/mission-stmt-rewriter.cpp`
- Matching expectations in `unittest/syntax/lowering/mission-stmt-rewriter.cpp`

This is correct but easy to get wrong (wrong `nullptr` position, wrong allocator) and reads obscurely at call sites.

**Idea:** Add a small, named helper on `ParserIR` (or on `ParserIR::Builder`), e.g. `ParserIR::create_label_only(const LabelDef* label, ArenaAllocator<> allocator)`, and use it in both implementation and tests so the intent is obvious and construction stays consistent.

### Rewriters gracefully handle missing commands

TODO e.g. if no GOTO_IF_TRUE use GOTO_IF_FALSE

---

## 3. Design

- Approach: named construction helper for label-only `ParserIR` (on `ParserIR` or `ParserIR::Builder`, as in the idea above); rewriters gracefully handle missing commands (TODO e.g. if no GOTO_IF_TRUE use GOTO_IF_FALSE).

---

## 4. Plan

- Touch points from the debts above: `ParserIR` (and/or Builder), `lib/syntax/lowering/mission-stmt-rewriter.cpp`, `unittest/syntax/lowering/mission-stmt-rewriter.cpp`, and other rewriters as the missing-command TODO applies.
- Out of scope follow-ups: none recorded beyond the two debts above.

---

## 5. Acceptance

- Label-only construction uses the named helper in implementation and tests (per the idea above).
- Missing-command rewriter handling lands as described by that TODO when addressed.
- Full unit binary green after changes: `cmake --build build --target gta3sc_unittest && ./build/unittest/gta3sc_unittest`.
