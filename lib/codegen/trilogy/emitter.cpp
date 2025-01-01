#include <cmath>
#include <gta3sc/codegen/trilogy/emitter.hpp>
using gta3sc::util::bit_cast;

namespace gta3sc::codegen::trilogy
{
constexpr auto datatype_eoal = std::byte{0};
constexpr auto datatype_i32 = std::byte{1};
constexpr auto datatype_var = std::byte{2};
constexpr auto datatype_lvar = std::byte{3};
constexpr auto datatype_i8 = std::byte{4};
constexpr auto datatype_i16 = std::byte{5};
constexpr auto datatype_float = std::byte{6};

CodeEmitter::CodeEmitter(uint32_t initial_capacity)
{
    this->buffer_reserve(initial_capacity);
}

auto CodeEmitter::offset() const -> uint32_t
{
    return this->curr_offset;
}

void CodeEmitter::buffer_clear()
{
    this->buffer.clear();
}

auto CodeEmitter::buffer_size() const -> uint32_t
{
    assert(buffer.size() <= std::numeric_limits<uint32_t>::max());
    return static_cast<uint32_t>(buffer.size());
}

auto CodeEmitter::buffer_capacity() const -> uint32_t
{
    assert(buffer.capacity() <= std::numeric_limits<uint32_t>::max());
    return static_cast<uint32_t>(buffer.capacity());
}

void CodeEmitter::buffer_reserve(uint32_t capacity)
{
    this->buffer.reserve(capacity);
}

auto CodeEmitter::emit_opcode(uint16_t opcode) -> CodeEmitter&
{
    return emit_raw_u16(opcode);
}

auto CodeEmitter::emit_opcode(int16_t command_id, bool not_flag) -> CodeEmitter&
{
    assert(command_id >= 0); // ensure hibit not set
    const uint16_t command_hibit = not_flag ? 0x8000 : 0;
    return emit_opcode(static_cast<uint16_t>(command_id) | command_hibit);
}

auto CodeEmitter::emit_eoal() -> CodeEmitter&
{
    return emit_raw_byte(datatype_eoal);
}

auto CodeEmitter::emit_int(int32_t value) -> CodeEmitter&
{
    if(value >= std::numeric_limits<int8_t>::min()
       && value <= std::numeric_limits<int8_t>::max())
    {
        return emit_i8(value);
    }
    else if(value >= std::numeric_limits<int16_t>::min()
            && value <= std::numeric_limits<int16_t>::max())
    {
        return emit_i16(value);
    }
    else
    {
        return emit_i32(value);
    }
}

auto CodeEmitter::emit_i8(int8_t value) -> CodeEmitter&
{
    emit_raw_byte(datatype_i8);
    emit_raw_i8(value);
    return *this;
}

auto CodeEmitter::emit_i16(int16_t value) -> CodeEmitter&
{
    emit_raw_byte(datatype_i16);
    emit_raw_i16(value);
    return *this;
}

auto CodeEmitter::emit_i32(int32_t value) -> CodeEmitter&
{
    emit_raw_byte(datatype_i32);
    emit_raw_i32(value);
    return *this;
}

auto CodeEmitter::emit_q11_4(float value) -> CodeEmitter&
{
    emit_raw_byte(datatype_float);
    emit_raw_i16(float_to_q11_4(value));
    return *this;
}

auto CodeEmitter::emit_var(uint16_t offset) -> CodeEmitter&
{
    emit_raw_byte(datatype_var);
    emit_raw_u16(offset);
    return *this;
}

auto CodeEmitter::emit_lvar(uint16_t index) -> CodeEmitter&
{
    emit_raw_byte(datatype_lvar);
    emit_raw_u16(index);
    return *this;
}

auto CodeEmitter::emit_raw_i8(int8_t value) -> CodeEmitter&
{
    return emit_raw_byte(bit_cast<std::byte>(value));
}

auto CodeEmitter::emit_raw_u8(uint8_t value) -> CodeEmitter&
{
    return emit_raw_byte(bit_cast<std::byte>(value));
}

auto CodeEmitter::emit_raw_i16(int16_t value) -> CodeEmitter&
{
    return emit_raw_u16(bit_cast<uint16_t>(value));
}

auto CodeEmitter::emit_raw_i32(int32_t value) -> CodeEmitter&
{
    return emit_raw_u32(bit_cast<uint32_t>(value));
}

auto CodeEmitter::emit_raw_byte(std::byte value) -> CodeEmitter&
{
    this->buffer.push_back(value);
    ++this->curr_offset;
    return *this;
}

auto CodeEmitter::emit_raw_u16(uint16_t value) -> CodeEmitter&
{
    const auto buffer_pos = this->buffer.size();
    this->buffer.resize(buffer_pos + 2);
    this->curr_offset += 2;

    buffer[buffer_pos] = bit_cast<std::byte>(
            static_cast<uint8_t>(value & 0x00FFU));

    buffer[buffer_pos + 1] = bit_cast<std::byte>(
            static_cast<uint8_t>((value & 0xFF00U) >> 8U));

    return *this;
}

auto CodeEmitter::emit_raw_u32(uint32_t value) -> CodeEmitter&
{
    const auto buffer_pos = this->buffer.size();
    this->buffer.resize(buffer_pos + 4);
    this->curr_offset += 4;

    buffer[buffer_pos] = bit_cast<std::byte>(
            static_cast<uint8_t>(value & 0x000000FFU));

    buffer[buffer_pos + 1] = bit_cast<std::byte>(
            static_cast<uint8_t>((value & 0x0000FF00U) >> 8U));

    buffer[buffer_pos + 2] = bit_cast<std::byte>(
            static_cast<uint8_t>((value & 0x00FF0000U) >> 16U));

    buffer[buffer_pos + 3] = bit_cast<std::byte>(
            static_cast<uint8_t>((value & 0xFF000000U) >> 24U));

    return *this;
}

auto CodeEmitter::float_to_q11_4(float value) const -> int16_t
{
    // https://en.wikipedia.org/wiki/Q_(number_format)

    constexpr float q11_4_min = -2048.0;
    constexpr float q11_4_max = 2047.9375;

    static_assert(q11_4_min == -0x1p+11F);
    static_assert(q11_4_max == 0x1.fffcp+10F);

    // According to [conv.fpint] (ISO C++), converting from float to an
    // integral is undefined behaviour if the truncated floating-point
    // cannot be represented in the destination type. Therefore, we
    // have to make sure `value` is within bounds of the destination
    // type (int16_t) before doing `static_cast`.
    //
    // To do this, we ensure the input value is within the bounds of
    // floating-point numbers representable in Q11.4. Any value
    // outside the range is transformed to the nearest representable value.
    value = std::fmax(q11_4_min, value); // filter NaN and out-of-bounds
    value = std::fmin(q11_4_max, value); // filter out-of-bounds

    // We cast to a 32-bit signed number instead of a 16-bit one just to
    // ensure the bounds of the fixed-point number with an assert.
    const auto q11_4 = static_cast<int32_t>(value * 16.0F);

    assert(q11_4 >= std::numeric_limits<int16_t>::min()
           && q11_4 <= std::numeric_limits<int16_t>::max());

    return static_cast<int16_t>(q11_4);
}
} // namespace gta3sc::codegen::trilogy
