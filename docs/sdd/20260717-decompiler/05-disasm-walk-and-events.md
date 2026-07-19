---
inherits: docs/sdd
spec_id: 20260717-decompiler
title: Disasm walk and events (delta)
status: approved
doc_role: stage-design-delta
---

# 5. Disasm walk and events

Delta design for the **public walk / event layer** and how `Ir2Printer` fits.
Core decode / analyze / multifile pieces remain in [03-design](03-design.md).

## 5.1 Why a walk + sink

`DisasmIR` is only a **successful command**. IR2 (and most tools) also need:

- **labels** (from `SegmentAnalysis`, not from `DisasmIR`)
- **undecoded byte runs** (analysis said “not code”)

So the library exposes a linear **walk** over a segment that pushes **events**
into a **sink**. IR2 text is one sink; other sinks can collect metrics, feed
tests, or (later) materialize an arena IR — without making IR2 the intermediate
language.

```text
SegmentAnalyzer ──▶ SegmentAnalysis
         │
SegmentWalker ──on_label / on_command / on_undecoded_bytes──▶ sink
         │
    Ir2Printer (sink) ──▶ emit_line(string_view)   // IR2 product API
```

## 5.2 Public API sketch

```cpp
namespace gta3sc::disasm
{
/// Receives one linear pass over a segment (sync; lifetime of bytes = call).
class DisasmEventSink
{
public:
    virtual ~DisasmEventSink() = default;

    virtual void on_label(uint32_t local_offset) = 0;

    virtual void on_command(const DisasmIR& insn) = 0;

    /// Bytes that were not decoded as a command. Open: prefer `span` at impl
    /// time; may rethink to (offset, length) paired with a buffer the sink
    /// already holds.
    virtual void on_undecoded_bytes(uint32_t local_offset,
                                    std::span<const std::byte> bytes) = 0;
};

class SegmentWalker
{
public:
    SegmentWalker(const InverseCommandTable& commands,
                  trilogy::DecodeOptions options);

    /// Walks [segment.base_offset, base_offset + size) in `bytecode`.
    void walk(std::span<const std::byte> bytecode,
              const trilogy::Segment& segment,
              const SegmentAnalysis& analysis,
              DisasmEventSink& sink);
};

namespace trilogy
{
/// Formats IR2 lines; constructed after label-id maps exist.
class Ir2Printer : public DisasmEventSink  // or adapts a sink; vtable OK for now
{
public:
    Ir2Printer(const InverseCommandTable& commands,
               const LabelIdMaps& label_ids, // main + per-block maps
               std::function_ref<void(std::string_view)> emit_line);

    void on_label(uint32_t local_offset) override;
    void on_command(const DisasmIR& insn) override;
    void on_undecoded_bytes(uint32_t local_offset,
                            std::span<const std::byte> bytes) override;
};
} // namespace trilogy
} // namespace gta3sc::disasm
```

**Vtable:** accepted for this phase for a clear library surface. Revisit later
(template sink / `function_ref` trio) if profiling warrants it.

## 5.3 `MultifileDisasm` orchestration

1. Parse header (or headerless single segment) → `Segment`s (`base_offset` +
   `size`).
2. `SegmentAnalyzer` on each segment (main labels collected for `@` refs).
3. Assign classic IR2 label ids (`MAIN_n`, `MISSION_b_n`) from sorted offsets.
4. Construct `Ir2Printer(commands, label_ids, emit_line)`.
5. Emit directives (`#DEFINE_MODEL`, `#MISSION_BLOCK_START/END`) with
   `emit_line` **from `MultifileDisasm`** (not via the walker).
6. For each segment: `SegmentWalker::walk(..., printer)`.

`driver::Decompilation` only wires `FilePool` / diagnostics and calls
`MultifileDisasm`.

## 5.4 Transactional `CodeDecoder::next()`

Success ⇒ complete `DisasmIR` (command + all args). Failure ⇒ `nullopt`.
Walk uses `seek` + `next()` at code offsets from analysis; undecoded ranges
become `on_undecoded_bytes` (IR2 prints `IR2_HEX` per byte).

How args are stored inside `DisasmIR` is an **implementation** choice; keep
allocations under control.

## 5.5 Relation to `InstructionVisitor`

Compiler `InstructionVisitor` targets arena `LinkedIR` nodes. This event sink is
a **push** API over a lazy walk — different shape. Do not force one visitor to
cover both. Future high-level uplift may materialize a `LinkedIR` and then use
rewriters/visitors; not required for IR2.
