#pragma once
#include <cassert>
#include <cstdint>
#include <gta3sc/util/bit_cast.hpp>
#include <string_view>
#include <vector>

namespace gta3sc::codegen::trilogy
{
/// Emits bytecode for GTA III, Vice City and San Andreas.
///
/// This is a raw bytecode emitter, It never fails and has no worries
/// with language semantics. It is only aware of bytecode representation.
///
/// Internally, it is backed by a buffer, on which emission occurs, and a
/// produced byte counter. The buffer can be drained at any time, to avoid
/// memory consumption. The byte counter is monotically increasing and
/// cannot be cleared.
///
/// Words are emitted in little-endian byte order.
///
/// For generating bytecode from IR, please use `CodeGen`.
class CodeEmitter
{
public:
    CodeEmitter() = default;

    /// Constructs an emitter with the given initial capacity in the buffer.
    explicit CodeEmitter(uint32_t initial_capacity);

    CodeEmitter(const CodeEmitter&) = delete;
    CodeEmitter& operator=(const CodeEmitter&) = delete;

    CodeEmitter(CodeEmitter&&) = default;
    CodeEmitter& operator=(CodeEmitter&&) = default;

    /// Returns the number of bytes produced thus far.
    auto offset() const -> uint32_t;

    /// Moves the content of the internal buffer into the given output iterator.
    ///
    /// Returns an iterator past-the-last element written to it.
    template<typename OutputIterator>
    auto drain(OutputIterator output_iter) -> OutputIterator;

    /// Clears the internal buffer.
    void buffer_clear();

    /// Returns the size of the internal buffer.
    auto buffer_size() const -> uint32_t;

    /// Returns the capacity of the internal buffer.
    auto buffer_capacity() const -> uint32_t;

    /// Ensures the internal buffer has at least `capacity` bytes of capacity.
    void buffer_reserve(uint32_t capacity);

    /// Emits the given 16-bit opcode into the buffer.
    auto emit_opcode(uint16_t opcode) -> CodeEmitter&;

    /// Emits a 16-bit opcode representing the given command index combined
    /// with a not_flag.
    auto emit_opcode(int16_t command_id, bool not_flag) -> CodeEmitter&;

    /// Emits a null byte representing the end-of-argument-list.
    auto emit_eoal() -> CodeEmitter&;

    /// Emits either an 8-bit, 16-bit or 32-bit integer argument, depending on
    /// the bounds of `value`.
    auto emit_int(int32_t value) -> CodeEmitter&;

    /// Emits an 8-bit integer argument (i.e. datatype byte + 8-bit value).
    auto emit_i8(int8_t value) -> CodeEmitter&;

    /// Emits a 16-bit integer argument (i.e. datatype byte + 16-bit value).
    auto emit_i16(int16_t value) -> CodeEmitter&;

    /// Emits a 32-bit integer argument (i.e. datatype byte + 32-bit value).
    auto emit_i32(int32_t value) -> CodeEmitter&;

    /// Emits a Q11.4 fixed-point argument (i.e. datatype byte + 16-bit value).
    ///
    /// In case the given floating-point cannot be represented as an Q11.4
    /// fixed-point number, the nearest representable number is used.
    auto emit_q11_4(float) -> CodeEmitter&;

    /// Emits a global integer/float variable offset (i.e. datatype byte +
    /// 16-bit offset).
    auto emit_var(uint16_t offset) -> CodeEmitter&;

    /// Emits a local integer/float variable index (i.e. datatype byte + 16-bit
    /// index).
    auto emit_lvar(uint16_t index) -> CodeEmitter&;

    /// Emits a 8-bit value with no datatype byte.
    auto emit_raw_byte(std::byte) -> CodeEmitter&;

    /// Emits a signed 8-bit value with no datatype byte.
    auto emit_raw_i8(int8_t) -> CodeEmitter&;

    /// Emits an unsigned 8-bit value with no datatype byte.
    auto emit_raw_u8(uint8_t) -> CodeEmitter&;

    /// Emits a signed 16-bit value with no datatype byte.
    auto emit_raw_i16(int16_t) -> CodeEmitter&;

    /// Emits an unsigned 16-bit value with no datatype byte.
    auto emit_raw_u16(uint16_t) -> CodeEmitter&;

    /// Emits a signed 32-bit value with no datatype byte.
    auto emit_raw_i32(int32_t) -> CodeEmitter&;

    /// Emits an unsigned 32-bit value with no datatype byte.
    auto emit_raw_u32(uint32_t) -> CodeEmitter&;

    /// Emits the given sequence of bytes, filling a total of `output_size`
    /// bytes in the internal buffer.
    ///
    /// The amount of input bytes (i.e. `data_size`) must be less or equal
    /// than `output_size`. The difference `output_size - data_size` is filled
    /// with null bytes.
    template<typename T>
    auto emit_raw_bytes(const T* data, size_t data_size, size_t output_size)
            -> CodeEmitter&;

    /// Same as `emit_raw_bytes(data, data_size, data_size)`.
    template<typename T>
    auto emit_raw_bytes(const T* data, size_t data_size) -> CodeEmitter&;

private:
    /// Converts a floating-point into an Q11.4 fixed-point.
    ///
    /// In case the given floating-point is NaN, infinite, or bigger than
    /// what can be stored in Q11.4, the nearest representable is returned.
    auto float_to_q11_4(float value) -> int16_t;

private:
    std::vector<std::byte> buffer;
    uint32_t curr_offset{};
};

template<typename OutputIterator>
inline auto CodeEmitter::drain(OutputIterator output_iter) -> OutputIterator
{
    for(const auto& byte_val : this->buffer)
        *output_iter++ = byte_val;

    this->buffer_clear();
    return output_iter;
}

template<typename T>
inline auto CodeEmitter::emit_raw_bytes(const T* data, size_t data_size,
                                        size_t output_size) -> CodeEmitter&
{
    using util::bit_cast;
    static_assert(sizeof(T) == sizeof(std::byte));

    assert(data_size <= output_size);

    const auto buffer_pos = this->buffer.size();
    this->buffer.resize(buffer_pos + output_size);
    this->curr_offset += output_size;

    size_t i = 0;

    for(; i < data_size && i < output_size; ++i)
        buffer[buffer_pos + i] = bit_cast<std::byte>(data[i]);

    for(; i < output_size; ++i)
        buffer[buffer_pos + i] = std::byte{0};

    return *this;
}

template<typename T>
inline auto CodeEmitter::emit_raw_bytes(const T* data, size_t data_size)
        -> CodeEmitter&
{
    return emit_raw_bytes(data, data_size, data_size);
}
} // namespace gta3sc::codegen::trilogy
