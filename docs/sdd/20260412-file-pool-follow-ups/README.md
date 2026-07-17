---
inherits: docs/sdd
spec_id: 20260412-file-pool-follow-ups
title: File pool follow-ups (paging, eviction, retention)
status: draft
doc_role: single-file
---

# Spec: File pool follow-ups (paging, eviction, retention)

**Source code is the source of truth.** This file is disposable scaffolding. Prefer merged code/tests on conflict; humans may delete this directory anytime.

**Status:** draft

Slim follow-up of remaining gaps from the agreed paging / retention / phasing design (`plans/archive/file-manager/solution-paging-retention-phasing.md`). Not a migration of the full file-manager archive or final-plan agent briefs.

---

## 1. Context

### Problem (what is still missing)

Paging is designed but **eviction / reload are not implemented**. Bytes stay resident after load (effectively **always resident** / `never_evict`). Snippet-style APIs that return `std::nullopt` when bytes are absent exist as the safety shape; **reload-from-path**, **mismatch detection**, and **parametrized eviction** remain open.

Config/models loaders still carry “page out when done” intent (early release of file bytes after load) that depends on paging + reload or `std::nullopt` — not delivered by eviction yet.

### Goals (from the agreed design)

1. **Bounded memory** for multi-file loads (XML imports, many IDEs, large trees) without breaking the **logical** location/range space already stored in IR, symbols, or any subsystem that stores ranges (each range is valid **only** relative to the **manager instance** that allocated it).
2. **No UB** when bytes are absent; **no silent “empty string” when materialization is impossible** — the snippet-style API should return **`std::nullopt`** (or `std::expected` with an error) so callers know text was unavailable. **Reload-from-path** is the preferred way to recover bytes when a stable path exists and policy allows.
3. **Reload detect mismatch**: on re-materialization, compare against **snapshot metadata** recorded at first load — **file size + `mtime`** when available; **no inode** in v1. If the file **changed**, treat as **unavailable** → **`std::nullopt`** (avoid showing text that may not match what the compiler saw).
4. **Stable outward API**, **evolving implementation**: start from a **single eviction policy** (effectively **always resident**), with **null-safe** snippet paths; then add **parametrized eviction** and reload behind the **same** public operations where possible.
5. **Clear policy** for what **refcount** guarantees (open handles) vs **pluggable eviction** vs **on-demand reload** — without a **driver-visible** pin and **without** the manager assuming **when** callers resolve ranges to text.

### Non-goals / deferred (keep in this follow-up; do not implement at first)

- **Soft pin** and **`compact()`** — out of scope until proven necessary. Prefer reload-centric eviction + optional **separate manager lifetimes**. Source code should document the **intent** to add pin/`compact()` if reload-centric eviction proves insufficient.
- Aggressive **`source_infos` / entry row caps** or recycling — low priority; unlikely to matter if each manager instance is **short-lived** (phase-scoped); revisit for long-lived processes only.
- DAT case-resolution, script `scan_directory` semantics, and large API rename — other file-manager rows; not this follow-up.

### Constraints (paging-relevant)

| Topic | Constraint |
| ----- | ---------- |
| Lifetime | After language parse, the open file handle is gone; IR/sema keep locations only. Range-to-bytes must go through the manager, not a dead handle. |
| Paging aggressiveness | DAT/XML-style loading: **many files** → aggressive page-out is attractive. Same-phase correctness is satisfied by **reload + mismatch detection + `std::nullopt`**, or by **bytes staying resident** while loaders run. |
| Refcount | Refcount tracks open handles only; **refcount = 0 does not imply “no one will ask for snippets.”** Eviction policy + reload determine what happens next. |
| Driver order | Config and DAT phases run **before** script directory scan in the intended driver. **Two manager lifetimes**: a location is valid only for the **manager instance** that allocated it. |

---

## 2. Requirements

### Resident vs paged-out

- **Resident**: the manager holds a **backing store** for that entry’s bytes — either a **heap buffer** or a **file mapping** (`mmap` / Win32 mapping). Callers do not see which.
- **Paged out**: metadata remains (`path` if known, `file_length`, global base location, **snapshot metadata for mismatch detection**, flags). **Backing store released.** Logical offsets remain valid for **identity**; **materializing** a span requires reload (or returns **`std::nullopt`** if impossible or policy says unavailable).
- **No path / ephemeral buffer**: **reload is impossible** after eviction → snippet API returns **`std::nullopt`** (no UB).

### Snippet API and repeated fetches

- **First** request for a range when bytes are **not** resident → **reload** (if path + policy allow), **verify mismatch metadata** → on success, attach resident backing until **eviction** for that row.
- **Further** requests **while still resident** → **serve from memory** — **do not** re-read the whole file from disk on every diagnostic line.
- After **eviction**, the next request pays **one** reload again (unless **`std::nullopt`**).

### Refcount

- **While refcount > 0**: bytes remain reachable through the live handle.
- **After refcount → 0**: eviction policy decides whether backing may be dropped. Snippet materialization may **reload** or return **`std::nullopt`**.
- Holding an open handle during `load_config` / `load_models_from_level` keeps **refcount > 0** for that call — no separate pin feature required for that.

### Eviction policy

**First implementation:** **never evict** (always resident after load) — still **null-safe** snippet paths and **`std::nullopt`** when bytes are missing.

**Later (exact set TBD), pluggable per manager instance**, examples:

- **`never_evict`** — default early behavior.
- **`lru_n`** or similar — cap **number of resident file bodies**; evict least-recently-used when over cap (refcount == 0 only; **never** evict while a handle is open).
- **`max_resident_bytes`** — cap approximate **total** resident backing; evict by LRU or similar when over cap.

**Byte-budget LRU (leading candidate after `never_evict`):** combine the **byte cap** with **LRU eviction among refcount == 0 rows**. Detailed catalog and knobs remain **TBD when eviction ships**.

Ordinary eviction (when policy != never): drop mmap/heap backing only when **refcount == 0** and policy selects that row; keep the entry **row** and logical range for the **lifetime of that manager instance** (monotonic metadata per instance).

### Reload, mismatch, `std::nullopt`

At **first** successful materialization, record **file size** and **`mtime`** when available. **Do not use inode / device ids in the first design.**

On reload:

- Metadata **disagrees** with current disk file → **`std::nullopt`**.
- Read/mmap **fails** → **`std::nullopt`**.
- **No path** → **`std::nullopt`** after eviction.

### Two managers (preferred direction)

DAT/XML (and similar) run with a **short-lived** manager instance (eventually eviction suited to **many small files**). When that phase completes, the instance is **destroyed (RAII)** — its locations become unusable unless something else has **copied** what it needs.

Normal compilation (mission scripts after directory scan) uses a **new** manager instance (often **never evict** or gentler LRU).

Phases need **not** share one global manager. **Two instances** ⇒ **two independent location spaces.** Code that resolves a range to bytes must use the **same** manager instance that issued the handle.

### Row growth

Monotonic entry rows are separate from **evicting byte backing**. With **phase-scoped** short-lived instances, a dedicated **row cap** or **recycling** scheme is **probably unnecessary**. Revisit only for a **long-lived** process holding one manager across many compilations or an extreme number of files in a **single** instance.

---

## 3. Design

### Approach A — Reload-centric (**preferred**)

- Eviction when policy allows and refcount == 0; materialize via reload + mismatch check → **`std::nullopt`** on failure or mismatch.
- Combine with **separate manager lifetimes** for data vs scripts where appropriate.
- **Pros:** minimal API surface; clear memory boundaries; aligns with parameterized eviction **without** pin/`compact()` initially.

### Approach B — Soft pin + `compact()` (**deferred**)

- **Not** implemented until evidence requires it.

### Approach C — Safety-first, single policy

- **First milestone:** **`never_evict`**; null-safe snippets; **`std::nullopt`** when bytes absent; **public API stable**.
- **Pros:** smallest step; no UB when data absent in edge paths.
- **Cons:** memory grows until eviction policies exist.

### Recommendation (milestones still open after the initial never-evict / null-safe shape)

1. **Milestone 1 (Approach C):** **`never_evict`** only; snippet materialization returns **`std::nullopt`** when bytes are missing; **stable** outward API; comments reserving **pin / `compact()`** for future need.
2. **Milestone 2:** pluggable **eviction policy** hook (`**never_evict`** first); **which** concrete strategies ship is **out of scope** until needed.
3. **Milestone 3:** **Approach A** fully wired — reload + mismatch + resident caching; **heap or mmap** internal; threading locks around transitions when multi-threaded.
4. **Architecture:** pursue **distinct manager instances** for **data/config** vs **scripts** with **RAII teardown** between phases **before** introducing pin/`compact()`; keep layering so **consumers** of locations stay paired with the right instance.
5. **Threading (future):** protect eviction, reload, refcount, policy bookkeeping with locking; **pin** was not locking-critical in v1 since deferred.

### Source code documentation (pin / `compact()`)

When introducing or refactoring the manager type, include a **short comment block** or module note:

- Eviction is **policy-driven**; **reload** materializes bytes when allowed.
- **`compact()`** and **pin** are **not** implemented initially; reserved if reload + eviction policy + scoped manager lifetime are insufficient.

No public API surface for pin/`compact()` until a follow-up explicitly adds it.

### mmap vs heap (implementation only)

Resident bytes are an **implementation detail**. Unix/Win32 may use **mmap**; platforms without mmap use **read into owned buffer**. The public contract is “optional bytes for this range,” not “mapping handle.” Both require explicit **release** for paging. Policy names and exact “max resident” semantics for mmap remain **TBD**; start with **never evict**.

---

## 4. Plan

### Remaining TODOs (from the paging design note)

| # | Item |
| - | ---- |
| T1 | Specify **snippet materialization** signature (`optional` / `expected`) and ensure **no** silent empty when unavailable. |
| T2 | Record **snapshot metadata** (**size + mtime**) on first load; implement **mismatch → `std::nullopt`** on reload. |
| T3 | Implement **resident caching**: one reload per resident epoch; no per-diagnostic full-file re-read. |
| T4 | Choose **heap vs mmap** per platform/file class **inside** manager; **no mmap in public headers**; non-mmap platform = read-all. |
| T5 | Add **locking** for refcount, eviction policy actions, reload (even if single-threaded v1). |
| T6 | **Eviction policy** interface: start with **`never_evict`** only; additional strategies (**LRU**, **byte cap**, etc.) when product needs them (catalog not fixed). |
| T7 | **Two-manager lifecycle** (data vs scripts): RAII scopes; document which subsystem owns which manager instance and that locations are **not** portable across instances. |
| T8 | **Comment / doc** reserving **pin** and **`compact()`** on manager type. |
| T9 | **Row cap / recycling** — defer; likely **unnecessary** for phase-scoped managers; revisit for long-lived or single-huge-arena use only. |
| T10 | Revisit **owned return type** for snippets if `string_view` lifetime becomes awkward after reload into scratch buffers. |

### High-level code impact (when implementing)

| Area | Likely change |
| ---- | ------------- |
| Manager / entry | Backing store + kind (heap vs mmap internal); resident flag; **snapshot metadata**; **eviction policy** hook; **reload** + mismatch check; **mutex** for state when concurrent. |
| Snippet / materialize API | Returns **`std::optional`** (or `expected`); **`std::nullopt`** for impossible reload, mismatch, no path — **not** silent empty string. |
| Open-handle / `data()` | Document: invalid or absent when paged out **unless** refcount keeps backing alive. |
| Eviction | Policy-driven drop of mmap/heap; **never** break refcount > 0. |
| Phasing | Optional **two** manager instances — data vs scripts; driver or coordinator owns RAII scopes. |
| Comments | Intent: **pin** / **`compact()`** may appear later if reload + policy + split managers are insufficient. |
| Config / models | Prefer **scoped** manager + policy for DAT/XML; avoid growing global singleton assumptions. Inline “page out … when done” TODOs align with this follow-up once eviction exists. |
| Tests | Ephemeral loads: **`std::nullopt`** after eviction without path; **`never_evict`** for many tests initially. |

### Touch points

- `include/gta3sc/filesystem/file-pool.hpp`, `lib/filesystem/file-pool.cpp` (and related location types)
- Config / models loaders that want early page-out after load
- Driver / coordinator for phase-scoped manager lifetimes
- Unit tests for eviction, reload, mismatch, ephemeral `nullopt`

Ignore for a long time is fine; this spec exists so the gaps remain written down.

---

## 5. Acceptance

Done when the remaining milestones / TODOs above that are chosen to ship are satisfied — especially:

- Snippet materialization returns **`std::nullopt`** (or `expected`) when unavailable — not a silent empty string.
- Eviction policy interface with **`never_evict`** first; further strategies when product needs them.
- Reload + size/`mtime` mismatch → **`std::nullopt`**; resident caching (one reload per resident epoch).
- Pin / `compact()` still not in the public API until an explicit follow-up; comments reserve the intent where the manager type is introduced/refactored.
- Full unit binary green: `cmake --build build --target gta3sc_unittest && ./build/unittest/gta3sc_unittest`.
