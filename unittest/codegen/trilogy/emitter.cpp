#include "../../stringification.hpp"
#include <cmath>
#include <doctest/doctest.h>
#include <gta3sc/codegen/trilogy/emitter.hpp>

namespace gta3sc::test::codegen::trilogy
{
class CodeEmitterFixture
{
};
} // namespace gta3sc::test::codegen::trilogy

using gta3sc::codegen::trilogy::CodeEmitter;
using gta3sc::test::codegen::trilogy::CodeEmitterFixture;
using std::back_inserter;

TEST_CASE_FIXTURE(CodeEmitterFixture, "initial emitter state")
{
    SUBCASE("emitter offset is zero") { REQUIRE(CodeEmitter().offset() == 0); }

    SUBCASE("internal buffer is empty")
    {
        REQUIRE(CodeEmitter().buffer_size() == 0);
    }

    SUBCASE("does not allocate internal buffer")
    {
        REQUIRE(CodeEmitter().buffer_capacity() == 0);
    }

    SUBCASE("allocates internal buffer if requested")
    {
        CodeEmitter emitter(32);
        REQUIRE(emitter.buffer_capacity() == 32);
        REQUIRE(emitter.buffer_size() == 0); // not affected
        REQUIRE(emitter.offset() == 0);      // not affected
    }
}

TEST_CASE_FIXTURE(CodeEmitterFixture, "internal buffer reserve")
{
    CodeEmitter emitter;

    CHECK(emitter.buffer_capacity() == 0);
    CHECK(emitter.buffer_size() == 0);
    CHECK(emitter.offset() == 0);

    emitter.buffer_reserve(32);

    REQUIRE(emitter.buffer_capacity() == 32);
    REQUIRE(emitter.buffer_size() == 0); // not affected
    REQUIRE(emitter.offset() == 0);      // not affected
}

TEST_CASE_FIXTURE(CodeEmitterFixture, "internal buffer size")
{
    CodeEmitter emitter;

    CHECK(emitter.buffer_size() == 0);

    emitter.emit_raw_byte(std::byte{0});

    REQUIRE(emitter.buffer_size() == 1);
}

TEST_CASE_FIXTURE(CodeEmitterFixture, "internal buffer clearing")
{
    CodeEmitter emitter(32);

    CHECK(emitter.buffer_capacity() == 32);
    CHECK(emitter.buffer_size() == 0);
    CHECK(emitter.offset() == 0);

    SUBCASE("cleaning empty buffer does nothing")
    {
        emitter.buffer_clear();
        REQUIRE(emitter.buffer_capacity() == 32);
        REQUIRE(emitter.buffer_size() == 0);
        REQUIRE(emitter.offset() == 0);
    }

    SUBCASE("cleaning buffer only resets buffer_size")
    {
        emitter.emit_raw_byte(std::byte{0});
        CHECK(emitter.buffer_size() == 1);

        emitter.buffer_clear();
        REQUIRE(emitter.offset() == 1);
        REQUIRE(emitter.buffer_capacity() == 32);
        REQUIRE(emitter.buffer_size() == 0);
    }
}

TEST_CASE_FIXTURE(CodeEmitterFixture, "internal buffer draining")
{
    CodeEmitter emitter(32);

    CHECK(emitter.buffer_capacity() == 32);
    CHECK(emitter.buffer_size() == 0);
    CHECK(emitter.offset() == 0);

    SUBCASE("draining empty buffer does nothing")
    {
        std::vector<std::byte> output;
        emitter.drain(back_inserter(output));

        CHECK(output.size() == 0);

        REQUIRE(emitter.buffer_capacity() == 32);
        REQUIRE(emitter.buffer_size() == 0);
        REQUIRE(emitter.offset() == 0);
    }

    SUBCASE("draining buffer")
    {
        std::vector<std::byte> output;
        emitter.emit_raw_byte(std::byte{1}).drain(back_inserter(output));

        SUBCASE("draining buffer only resets buffer_size")
        {
            REQUIRE(emitter.offset() == 1);
            REQUIRE(emitter.buffer_capacity() == 32);
            REQUIRE(emitter.buffer_size() == 0);
        }

        SUBCASE("draining buffer outputs emitted bytes")
        {
            REQUIRE(output == std::vector{std::byte{1}});
        }
    }
}

TEST_CASE_FIXTURE(CodeEmitterFixture, "emit opcode")
{
    CodeEmitter emitter;
    emitter.emit_opcode(0x1234);

    SUBCASE("output is 16-bit little-endian")
    {
        std::vector<std::byte> output;
        emitter.drain(back_inserter(output));
        REQUIRE(output == std::vector{std::byte{0x34}, std::byte{0x12}});
    }

    SUBCASE("emitting opcode increases offset by 2")
    {
        REQUIRE(emitter.offset() == 2);
    }
}

TEST_CASE_FIXTURE(CodeEmitterFixture,
                  "emit opcode from command_id and not_flag=0")
{
    CodeEmitter emitter;
    emitter.emit_opcode(4660, false);

    CHECK(emitter.offset() == 2);

    SUBCASE("output has hibit of 16-bit word unset")
    {
        std::vector<std::byte> output;
        emitter.drain(back_inserter(output));
        REQUIRE(output == std::vector{std::byte{0x34}, std::byte{0x12}});
    }
}

TEST_CASE_FIXTURE(CodeEmitterFixture,
                  "emit opcode from command_id and not_flag=1")
{
    CodeEmitter emitter;
    emitter.emit_opcode(4660, true);

    CHECK(emitter.offset() == 2);

    SUBCASE("output has hibit of 16-bit word set")
    {
        std::vector<std::byte> output;
        emitter.drain(back_inserter(output));
        REQUIRE(output == std::vector{std::byte{0x34}, std::byte{0x92}});
    }
}

TEST_CASE_FIXTURE(CodeEmitterFixture, "emit end-of-argument-list")
{
    CodeEmitter emitter;
    emitter.emit_eoal();

    SUBCASE("output is null byte")
    {
        std::vector<std::byte> output;
        emitter.drain(back_inserter(output));
        REQUIRE(output == std::vector{std::byte{0}});
    }

    SUBCASE("increases offset by 1") { REQUIRE(emitter.offset() == 1); }
}

TEST_CASE_FIXTURE(CodeEmitterFixture, "emit 8-bit integer argument")
{
    CodeEmitter emitter;
    emitter.emit_i8(1);

    SUBCASE("produces datatype and 8-bit byte")
    {
        std::vector<std::byte> output;
        emitter.drain(back_inserter(output));
        REQUIRE(output == std::vector{std::byte{0x04}, std::byte{0x01}});
    }

    SUBCASE("increases offset by 2") { REQUIRE(emitter.offset() == 2); }

    SUBCASE("negative 8-bit value uses two-complement representation")
    {
        std::vector<std::byte> output;
        CodeEmitter().emit_i8(-2).drain(back_inserter(output));
        REQUIRE(output == std::vector{std::byte{0x04}, std::byte{0xFE}});
    }
}

TEST_CASE_FIXTURE(CodeEmitterFixture, "emit 16-bit integer argument")
{
    CodeEmitter emitter;
    emitter.emit_i16(1);

    SUBCASE("produces datatype and 16-bit little-endian byte sequence")
    {
        std::vector<std::byte> output;
        emitter.drain(back_inserter(output));
        REQUIRE(output
                == std::vector{std::byte{0x05}, std::byte{0x01},
                               std::byte{0x00}});
    }

    SUBCASE("increases offset by 3") { REQUIRE(emitter.offset() == 3); }

    SUBCASE("negative 16-bit value uses two-complement representation")
    {
        std::vector<std::byte> output;
        CodeEmitter().emit_i16(-2).drain(back_inserter(output));
        REQUIRE(output
                == std::vector{std::byte{0x05}, std::byte{0xFE},
                               std::byte{0xFF}});
    }
}

TEST_CASE_FIXTURE(CodeEmitterFixture, "emit 32-bit integer argument")
{
    CodeEmitter emitter;
    emitter.emit_i32(66051);

    SUBCASE("produces datatype and 32-bit little-endian byte sequence")
    {
        std::vector<std::byte> output;
        emitter.drain(back_inserter(output));
        REQUIRE(output
                == std::vector{std::byte{0x01}, std::byte{0x03},
                               std::byte{0x02}, std::byte{0x01},
                               std::byte{0x00}});
    }

    SUBCASE("increases offset by 5") { REQUIRE(emitter.offset() == 5); }

    SUBCASE("negative 32-bit value uses two-complement representation")
    {
        std::vector<std::byte> output;
        CodeEmitter().emit_i32(-2).drain(back_inserter(output));
        REQUIRE(output
                == std::vector{std::byte{0x01}, std::byte{0xFE},
                               std::byte{0xFF}, std::byte{0xFF},
                               std::byte{0xFF}});
    }
}

TEST_CASE_FIXTURE(CodeEmitterFixture, "emit integer argument")
{
    CodeEmitter emitter;

    SUBCASE("produces 8-bit integer argument if value <=127")
    {
        std::vector<std::byte> output;
        emitter.emit_int(127).drain(back_inserter(output));
        REQUIRE(output == std::vector{std::byte{0x04}, std::byte{0x7F}});
        CHECK(emitter.offset() == 2);
    }

    SUBCASE("produces 8-bit integer argument if value >=-128")
    {
        std::vector<std::byte> output;
        emitter.emit_int(-128).drain(back_inserter(output));
        REQUIRE(output == std::vector{std::byte{0x04}, std::byte{0x80}});
        CHECK(emitter.offset() == 2);
    }

    SUBCASE("produces 16-bit integer argument if value 127+1")
    {
        std::vector<std::byte> output;
        emitter.emit_int(128).drain(back_inserter(output));
        REQUIRE(output
                == std::vector{std::byte{0x05}, std::byte{0x80},
                               std::byte{0x00}});
        CHECK(emitter.offset() == 3);
    }

    SUBCASE("produces 16-bit integer argument if value -128-1")
    {
        std::vector<std::byte> output;
        emitter.emit_int(-129).drain(back_inserter(output));
        REQUIRE(output
                == std::vector{std::byte{0x05}, std::byte{0x7F},
                               std::byte{0xFF}});
        CHECK(emitter.offset() == 3);
    }

    SUBCASE("produces 16-bit integer argument if value <=32767")
    {
        std::vector<std::byte> output;
        emitter.emit_int(32767).drain(back_inserter(output));
        REQUIRE(output
                == std::vector{std::byte{0x05}, std::byte{0xFF},
                               std::byte{0x7F}});
        CHECK(emitter.offset() == 3);
    }

    SUBCASE("produces 16-bit integer argument if value >=-32768")
    {
        std::vector<std::byte> output;
        emitter.emit_int(-32768).drain(back_inserter(output));
        REQUIRE(output
                == std::vector{std::byte{0x05}, std::byte{0x00},
                               std::byte{0x80}});
        CHECK(emitter.offset() == 3);
    }

    SUBCASE("produces 32-bit integer argument if value 32767+1")
    {
        std::vector<std::byte> output;
        emitter.emit_int(32768).drain(back_inserter(output));
        REQUIRE(output
                == std::vector{std::byte{0x01}, std::byte{0x00},
                               std::byte{0x80}, std::byte{0x00},
                               std::byte{0x00}});
        CHECK(emitter.offset() == 5);
    }

    SUBCASE("produces 32-bit integer argument if value -32768-1")
    {
        std::vector<std::byte> output;
        emitter.emit_int(-32769).drain(back_inserter(output));
        REQUIRE(output
                == std::vector{std::byte{0x01}, std::byte{0xFF},
                               std::byte{0x7F}, std::byte{0xFF},
                               std::byte{0xFF}});
        CHECK(emitter.offset() == 5);
    }
}

TEST_CASE_FIXTURE(CodeEmitterFixture, "emit Q11.4 fixed-point argument")
{
    constexpr float q11_4_resolution = 0.0625F;

    CodeEmitter emitter;

    SUBCASE("emits datatype and 16-bit fixed-point value")
    {
        std::vector<std::byte> output;
        emitter.emit_q11_4(0.0625).drain(back_inserter(output));
        REQUIRE(output
                == std::vector{std::byte{0x06}, std::byte{0x01},
                               std::byte{0x00}});
    }

    SUBCASE("negative fixed-point uses two complement")
    {
        std::vector<std::byte> output;
        emitter.emit_q11_4(-0.0625).drain(back_inserter(output));
        REQUIRE(output
                == std::vector{std::byte{0x06}, std::byte{0xFF},
                               std::byte{0xFF}});
    }

    SUBCASE("increases offset by 3")
    {
        std::vector<std::byte> output;
        emitter.emit_q11_4(0.0).drain(back_inserter(output));
        REQUIRE(emitter.offset() == 3);
    }

    SUBCASE("fixed-point number changes per resolution")
    {
        std::vector<std::byte> output;

        CodeEmitter()
                .emit_q11_4(-3 * q11_4_resolution)
                .drain(back_inserter(output));
        REQUIRE(output
                == std::vector{std::byte{0x06}, std::byte{0xFD},
                               std::byte{0xFF}});

        output.clear();
        CodeEmitter()
                .emit_q11_4(-2 * q11_4_resolution)
                .drain(back_inserter(output));
        REQUIRE(output
                == std::vector{std::byte{0x06}, std::byte{0xFE},
                               std::byte{0xFF}});

        output.clear();
        CodeEmitter()
                .emit_q11_4(-1 * q11_4_resolution)
                .drain(back_inserter(output));
        REQUIRE(output
                == std::vector{std::byte{0x06}, std::byte{0xFF},
                               std::byte{0xFF}});

        output.clear();
        CodeEmitter()
                .emit_q11_4(0 * q11_4_resolution)
                .drain(back_inserter(output));
        REQUIRE(output
                == std::vector{std::byte{0x06}, std::byte{0x00},
                               std::byte{0x00}});

        output.clear();
        CodeEmitter()
                .emit_q11_4(1 * q11_4_resolution)
                .drain(back_inserter(output));
        REQUIRE(output
                == std::vector{std::byte{0x06}, std::byte{0x01},
                               std::byte{0x00}});

        output.clear();
        CodeEmitter()
                .emit_q11_4(2 * q11_4_resolution)
                .drain(back_inserter(output));
        REQUIRE(output
                == std::vector{std::byte{0x06}, std::byte{0x02},
                               std::byte{0x00}});

        output.clear();
        CodeEmitter()
                .emit_q11_4(3 * q11_4_resolution)
                .drain(back_inserter(output));
        REQUIRE(output
                == std::vector{std::byte{0x06}, std::byte{0x03},
                               std::byte{0x00}});
    }

    SUBCASE("positive floating-points are rounded to the lowest nearest "
            "fixed-point")
    {
        const auto base_float = q11_4_resolution * 8.0F;

        const auto fixedpoint_7 = std::vector{std::byte{0x06}, std::byte{0x07},
                                              std::byte{0x00}};
        const auto fixedpoint_8 = std::vector{std::byte{0x06}, std::byte{0x08},
                                              std::byte{0x00}};

        std::vector<std::byte> output;

        CodeEmitter().emit_q11_4(base_float).drain(back_inserter(output));
        REQUIRE(output == fixedpoint_8);

        output.clear();
        CodeEmitter()
                .emit_q11_4(base_float + (q11_4_resolution / 2))
                .drain(back_inserter(output));
        REQUIRE(output == fixedpoint_8);

        output.clear();
        CodeEmitter()
                .emit_q11_4(base_float - (q11_4_resolution / 2))
                .drain(back_inserter(output));
        REQUIRE(output == fixedpoint_7);
    }

    SUBCASE("negative floating-points are rounded to the highest nearest "
            "fixed-point")
    {
        const auto base_float = -(q11_4_resolution * 8.0F);

        const auto fixedpoint_minus_7 = std::vector{
                std::byte{0x06}, std::byte{0xF9}, std::byte{0xFF}};
        const auto fixedpoint_minus_8 = std::vector{
                std::byte{0x06}, std::byte{0xF8}, std::byte{0xFF}};

        std::vector<std::byte> output;

        CodeEmitter().emit_q11_4(base_float).drain(back_inserter(output));
        REQUIRE(output == fixedpoint_minus_8);

        output.clear();
        CodeEmitter()
                .emit_q11_4(base_float + (q11_4_resolution / 2))
                .drain(back_inserter(output));
        REQUIRE(output == fixedpoint_minus_7);

        output.clear();
        CodeEmitter()
                .emit_q11_4(base_float - (q11_4_resolution / 2))
                .drain(back_inserter(output));
        REQUIRE(output == fixedpoint_minus_8);
    }

    SUBCASE("floating-point >2047.9375 cannot be represented in Q11.4 and "
            "maximum Q11.4 is used")
    {
        std::vector<std::byte> output;

        emitter.emit_q11_4(2047.9376).drain(back_inserter(output));
        REQUIRE(output
                == std::vector{std::byte{0x06}, std::byte{0xFF},
                               std::byte{0x7F}});

        output.clear();
        emitter.emit_q11_4(3000.0).drain(back_inserter(output));
        REQUIRE(output
                == std::vector{std::byte{0x06}, std::byte{0xFF},
                               std::byte{0x7F}});

        output.clear();
        emitter.emit_q11_4(INFINITY).drain(back_inserter(output));
        REQUIRE(output
                == std::vector{std::byte{0x06}, std::byte{0xFF},
                               std::byte{0x7F}});
    }

    SUBCASE("floating-point <-2048.0 cannot be represented in Q11.4 and "
            "minimum Q11.4 is used")
    {
        std::vector<std::byte> output;

        emitter.emit_q11_4(-2048.01).drain(back_inserter(output));
        REQUIRE(output
                == std::vector{std::byte{0x06}, std::byte{0x00},
                               std::byte{0x80}});

        output.clear();
        emitter.emit_q11_4(-3000.0).drain(back_inserter(output));
        REQUIRE(output
                == std::vector{std::byte{0x06}, std::byte{0x00},
                               std::byte{0x80}});

        output.clear();
        emitter.emit_q11_4(-INFINITY).drain(back_inserter(output));
        REQUIRE(output
                == std::vector{std::byte{0x06}, std::byte{0x00},
                               std::byte{0x80}});
    }

    SUBCASE("floating-point <=2047.9375 can still be represented in Q11.4")
    {
        std::vector<std::byte> output;
        emitter.emit_q11_4(2047.9375).drain(back_inserter(output));
        REQUIRE(output
                == std::vector{std::byte{0x06}, std::byte{0xFF},
                               std::byte{0x7F}});
    }

    SUBCASE("floating-point >=-2048.0 can still be represented in Q11.4")
    {
        std::vector<std::byte> output;
        emitter.emit_q11_4(-2048.0).drain(back_inserter(output));
        REQUIRE(output
                == std::vector{std::byte{0x06}, std::byte{0x00},
                               std::byte{0x80}});
    }
}

TEST_CASE_FIXTURE(CodeEmitterFixture, "emit local-variable index")
{
    CodeEmitter emitter;
    emitter.emit_lvar(1);

    SUBCASE("produces datatype and 16-bit little-endian byte sequence")
    {
        std::vector<std::byte> output;
        emitter.drain(back_inserter(output));
        REQUIRE(output
                == std::vector{std::byte{0x03}, std::byte{0x01},
                               std::byte{0x00}});
    }

    SUBCASE("increases offset by 3") { REQUIRE(emitter.offset() == 3); }
}

TEST_CASE_FIXTURE(CodeEmitterFixture, "emit global-variable offset")
{
    CodeEmitter emitter;
    emitter.emit_var(1);

    SUBCASE("produces datatype and 16-bit little-endian byte sequence")
    {
        std::vector<std::byte> output;
        emitter.drain(back_inserter(output));
        REQUIRE(output
                == std::vector{std::byte{0x02}, std::byte{0x01},
                               std::byte{0x00}});
    }

    SUBCASE("increases offset by 3") { REQUIRE(emitter.offset() == 3); }
}

TEST_CASE_FIXTURE(CodeEmitterFixture, "emit raw byte")
{
    CodeEmitter emitter;
    emitter.emit_raw_byte(std::byte{1});

    SUBCASE("produces a single byte")
    {
        std::vector<std::byte> output;
        emitter.drain(back_inserter(output));
        REQUIRE(output == std::vector{std::byte{1}});
    }

    SUBCASE("increases offset by 1") { REQUIRE(emitter.offset() == 1); }
}

TEST_CASE_FIXTURE(CodeEmitterFixture, "emit raw unsigned 8-bit value")
{
    CodeEmitter emitter;
    emitter.emit_raw_u8(1);

    SUBCASE("produces a single byte")
    {
        std::vector<std::byte> output;
        emitter.drain(back_inserter(output));
        REQUIRE(output == std::vector{std::byte{1}});
    }

    SUBCASE("increases offset by 1") { REQUIRE(emitter.offset() == 1); }
}

TEST_CASE_FIXTURE(CodeEmitterFixture, "emit raw signed 8-bit value")
{
    CodeEmitter emitter;
    emitter.emit_raw_i8(1);

    SUBCASE("produces 8-bit byte")
    {
        std::vector<std::byte> output;
        emitter.drain(back_inserter(output));
        REQUIRE(output == std::vector{std::byte{0x01}});
    }

    SUBCASE("increases offset by 1") { REQUIRE(emitter.offset() == 1); }

    SUBCASE("negative 8-bit value uses two-complement representation")
    {
        std::vector<std::byte> output;
        CodeEmitter().emit_raw_i8(-2).drain(back_inserter(output));
        REQUIRE(output == std::vector{std::byte{0xFE}});
    }
}

TEST_CASE_FIXTURE(CodeEmitterFixture, "emit raw unsigned 16-bit value")
{
    CodeEmitter emitter;
    emitter.emit_raw_u16(1);

    SUBCASE("produces 16-bit little-endian byte sequence")
    {
        std::vector<std::byte> output;
        emitter.drain(back_inserter(output));
        REQUIRE(output == std::vector{std::byte{0x01}, std::byte{0x00}});
    }

    SUBCASE("increases offset by 2") { REQUIRE(emitter.offset() == 2); }
}

TEST_CASE_FIXTURE(CodeEmitterFixture, "emit raw signed 16-bit value")
{
    CodeEmitter emitter;
    emitter.emit_raw_i16(1);

    SUBCASE("produces 16-bit little-endian byte sequence")
    {
        std::vector<std::byte> output;
        emitter.drain(back_inserter(output));
        REQUIRE(output == std::vector{std::byte{0x01}, std::byte{0x00}});
    }

    SUBCASE("increases offset by 2") { REQUIRE(emitter.offset() == 2); }

    SUBCASE("negative 16-bit value uses two-complement representation")
    {
        std::vector<std::byte> output;
        CodeEmitter().emit_raw_i16(-2).drain(back_inserter(output));
        REQUIRE(output == std::vector{std::byte{0xFE}, std::byte{0xFF}});
    }
}

TEST_CASE_FIXTURE(CodeEmitterFixture, "emit raw unsigned 32-bit value")
{
    CodeEmitter emitter;
    emitter.emit_raw_u32(66051);

    SUBCASE("produces 32-bit little-endian byte sequence")
    {
        std::vector<std::byte> output;
        emitter.drain(back_inserter(output));
        REQUIRE(output
                == std::vector{std::byte{0x03}, std::byte{0x02},
                               std::byte{0x01}, std::byte{0x00}});
    }

    SUBCASE("increases offset by 4") { REQUIRE(emitter.offset() == 4); }
}

TEST_CASE_FIXTURE(CodeEmitterFixture, "emit raw signed 32-bit value")
{
    CodeEmitter emitter;
    emitter.emit_raw_i32(66051);

    SUBCASE("produces 32-bit little-endian byte sequence")
    {
        std::vector<std::byte> output;
        emitter.drain(back_inserter(output));
        REQUIRE(output
                == std::vector{std::byte{0x03}, std::byte{0x02},
                               std::byte{0x01}, std::byte{0x00}});
    }

    SUBCASE("increases offset by 4") { REQUIRE(emitter.offset() == 4); }

    SUBCASE("negative 32-bit value uses two-complement representation")
    {
        std::vector<std::byte> output;
        CodeEmitter().emit_raw_i32(-2).drain(back_inserter(output));
        REQUIRE(output
                == std::vector{std::byte{0xFE}, std::byte{0xFF},
                               std::byte{0xFF}, std::byte{0xFF}});
    }
}

TEST_CASE_FIXTURE(CodeEmitterFixture, "emit raw bytes")
{
    std::vector raw_seq{std::byte{0x01}, std::byte{0x02}, std::byte{0x03}};
    CodeEmitter emitter;

    SUBCASE("produces null bytes when output_size is different from data size")
    {
        constexpr size_t num_null_bytes = 3;

        std::vector<std::byte> output;

        std::vector<std::byte> expected(raw_seq);
        expected.resize(expected.size() + num_null_bytes, std::byte{0});

        emitter.emit_raw_bytes(raw_seq.begin(), raw_seq.end(),
                               raw_seq.size() + num_null_bytes)
                .drain(back_inserter(output));

        REQUIRE(output == expected);
    }

    SUBCASE("produces no null bytes when output_size is the same as data size")
    {
        std::vector<std::byte> output;

        emitter.emit_raw_bytes(raw_seq.begin(), raw_seq.end(), raw_seq.size())
                .drain(back_inserter(output));
        REQUIRE(output == raw_seq);

        output.clear();
        emitter.emit_raw_bytes(raw_seq.begin(), raw_seq.end())
                .drain(back_inserter(output));
        REQUIRE(output == raw_seq);
    }

    SUBCASE("overload omitting output_size is same as passing data_size")
    {
        std::vector<std::byte> output1;
        std::vector<std::byte> output2;

        emitter.emit_raw_bytes(raw_seq.begin(), raw_seq.end(), raw_seq.size())
                .drain(back_inserter(output1));

        emitter.emit_raw_bytes(raw_seq.begin(), raw_seq.end())
                .drain(back_inserter(output2));

        REQUIRE(output1 == output2);
    }

    SUBCASE("increases offset by output_size")
    {
        emitter.emit_raw_bytes(raw_seq.begin(), raw_seq.end(), 100);
        CHECK(raw_seq.size() != 100);
        REQUIRE(emitter.offset() == 100);
    }
}

TEST_CASE_FIXTURE(CodeEmitterFixture, "offset accumulates through calls")
{
    CodeEmitter emitter;
    CHECK(emitter.offset() == 0);

    emitter.emit_raw_byte(std::byte{0});
    REQUIRE(emitter.offset() == 1);

    emitter.emit_raw_byte(std::byte{0});
    REQUIRE(emitter.offset() == 2);
}
