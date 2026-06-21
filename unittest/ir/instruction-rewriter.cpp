#include "../syntax/syntax-fixture.hpp"
#include <doctest/doctest.h>
#include <gta3sc/ir/instruction-rewriter.hpp>
#include <gta3sc/ir/parser-ir.hpp>
using namespace gta3sc::test::syntax; // NOLINT

namespace
{
using IR = gta3sc::ParserIR;

/// An instruction rewriter that never rewrites.
class TestNullRewriter final : public gta3sc::InstructionRewriter<IR>
{
public:
    auto visit(const IR&) -> Result override { return std::nullopt; }
};

/// An instruction rewriter that always deletes the visited line.
class TestDeleteRewriter final : public gta3sc::InstructionRewriter<IR>
{
public:
    auto visit(const IR&) -> Result override { return gta3sc::LinkedIR<IR>{}; }
};

/// An instruction rewriter that unconditionally replaces the visited line with
/// a single `NOP` command.
class TestReplaceWithNopRewriter final : public gta3sc::InstructionRewriter<IR>
{
public:
    explicit TestReplaceWithNopRewriter(gta3sc::ArenaAllocator<> alloc) :
        alloc(alloc)
    {}

    auto visit(const IR&) -> Result override
    {
        return gta3sc::LinkedIR<IR>{IR::Builder(alloc).command("NOP").build()};
    }

private:
    gta3sc::ArenaAllocator<> alloc;
};

/// An instruction rewriter that replaces a specific command with another.
class TestCommandRewriter final : public gta3sc::InstructionRewriter<IR>
{
public:
    TestCommandRewriter(gta3sc::ArenaAllocator<> alloc, std::string_view from,
                        std::string_view to) :
        alloc(alloc), from(from), to(to)
    {}

    auto visit(const IR& line) -> Result override
    {
        if(line.has_command() && line.command().name() == from)
            return gta3sc::LinkedIR<IR>{IR::Builder(alloc).command(to).build()};
        return std::nullopt;
    }

private:
    gta3sc::ArenaAllocator<> alloc;
    std::string_view from;
    std::string_view to;
};

/// An instruction rewriter that replaces `WAIT` with `NOP` followed by `GOTO`.
class TestExpandRewriter final : public gta3sc::InstructionRewriter<IR>
{
public:
    explicit TestExpandRewriter(gta3sc::ArenaAllocator<> alloc) : alloc(alloc)
    {}

    auto visit(const IR& line) -> Result override
    {
        if(line.has_command() && line.command().name() == "WAIT")
            return gta3sc::LinkedIR<IR>{
                    IR::Builder(alloc).command("NOP").build(),
                    IR::Builder(alloc).command("GOTO").build()};
        return std::nullopt;
    }

private:
    gta3sc::ArenaAllocator<> alloc;
};

/// An instruction rewriter that deletes every `GOTO` line.
class TestCommandDeleteRewriter final : public gta3sc::InstructionRewriter<IR>
{
public:
    auto visit(const IR& line) -> Result override
    {
        if(line.has_command() && line.command().name() == "GOTO")
            return gta3sc::LinkedIR<IR>{};
        return std::nullopt;
    }
};

class RewriterFixture : public SyntaxFixture
{
protected:
    auto cmd(std::string_view name) -> gta3sc::ArenaPtr<IR>
    {
        return IR::Builder(&arena).command(name).build();
    }

protected:
    gta3sc::ArenaMemoryResource arena{}; // NOLINT
};
} // namespace

TEST_CASE_FIXTURE(RewriterFixture, "rewrite with no rewrite does nothing")
{
    TestNullRewriter rewriter;
    auto input_ir = gta3sc::LinkedIR<IR>{cmd("WAIT"), cmd("GOTO")};

    auto it = rewriter.rewrite(input_ir, input_ir.begin());

    REQUIRE(it == input_ir.begin());
    CHECK(input_ir == gta3sc::LinkedIR<IR>{cmd("WAIT"), cmd("GOTO")});
}

TEST_CASE_FIXTURE(RewriterFixture, "rewrite with delete removes the node")
{
    TestDeleteRewriter rewriter;
    auto input_ir = gta3sc::LinkedIR<IR>{cmd("WAIT"), cmd("GOTO"),
                                         cmd("RETURN")};

    auto it = rewriter.rewrite(input_ir, input_ir.begin());

    REQUIRE(it == input_ir.begin());
    REQUIRE(it != input_ir.end());
    REQUIRE((*it).command().name() == "GOTO");
    CHECK(input_ir == gta3sc::LinkedIR<IR>{cmd("GOTO"), cmd("RETURN")});
}

TEST_CASE_FIXTURE(RewriterFixture, "rewrite with replacement splices in place "
                                   "and returns first replacement")
{
    TestReplaceWithNopRewriter rewriter(&arena);
    auto input_ir = gta3sc::LinkedIR<IR>{cmd("WAIT"), cmd("GOTO")};

    auto it = rewriter.rewrite(input_ir, input_ir.begin());

    REQUIRE((*it).command().name() == "NOP");
    CHECK(input_ir == gta3sc::LinkedIR<IR>{cmd("NOP"), cmd("GOTO")});
}

TEST_CASE_FIXTURE(RewriterFixture,
                  "rewrite_each with pass through keeps all nodes")
{
    TestNullRewriter rewriter;
    auto input_ir = gta3sc::LinkedIR<IR>{cmd("WAIT"), cmd("GOTO")};

    rewriter.rewrite_each(input_ir);

    CHECK(input_ir == gta3sc::LinkedIR<IR>{cmd("WAIT"), cmd("GOTO")});
}

TEST_CASE_FIXTURE(RewriterFixture, "rewrite_each with delete removes all nodes")
{
    TestDeleteRewriter rewriter;
    auto input_ir = gta3sc::LinkedIR<IR>{cmd("WAIT"), cmd("GOTO")};

    rewriter.rewrite_each(input_ir);

    CHECK(input_ir.empty());
}

TEST_CASE_FIXTURE(RewriterFixture,
                  "rewrite_each with replacement rewrites matching line")
{
    TestCommandRewriter rewriter(&arena, "WAIT", "NOP");
    auto input_ir = gta3sc::LinkedIR<IR>{cmd("WAIT"), cmd("GOTO")};

    rewriter.rewrite_each(input_ir);

    CHECK(input_ir == gta3sc::LinkedIR<IR>{cmd("NOP"), cmd("GOTO")});
}

TEST_CASE_FIXTURE(RewriterFixture, "rewrite_each with selective delete")
{
    TestCommandDeleteRewriter rewriter;
    auto input_ir = gta3sc::LinkedIR<IR>{cmd("WAIT"), cmd("GOTO"),
                                         cmd("RETURN")};

    rewriter.rewrite_each(input_ir);

    CHECK(input_ir == gta3sc::LinkedIR<IR>{cmd("WAIT"), cmd("RETURN")});
}

TEST_CASE_FIXTURE(RewriterFixture,
                  "CompositeInstructionRewriter visit with single rewriter")
{
    auto result
            = gta3sc::CompositeInstructionRewriter{TestReplaceWithNopRewriter(
                                                           &arena)}
                      .visit(*cmd("WAIT"));

    REQUIRE(result);
    CHECK(*result == gta3sc::LinkedIR<IR>{cmd("NOP")});
}

TEST_CASE_FIXTURE(RewriterFixture,
                  "CompositeInstructionRewriter visit chains rewriters")
{
    auto result
            = gta3sc::CompositeInstructionRewriter{TestReplaceWithNopRewriter(
                                                           &arena),
                                                   TestCommandRewriter(&arena,
                                                                       "NOP",
                                                                       "GOTO")}
                      .visit(*cmd("WAIT"));

    REQUIRE(result);
    CHECK(*result == gta3sc::LinkedIR<IR>{cmd("GOTO")});
}

TEST_CASE_FIXTURE(RewriterFixture, "CompositeInstructionRewriter visit passes "
                                   "through when no rewriter fires")
{
    auto result = gta3sc::CompositeInstructionRewriter{TestNullRewriter{},
                                                       TestNullRewriter{}}
                          .visit(*cmd("WAIT"));

    REQUIRE(!result);
}

TEST_CASE_FIXTURE(RewriterFixture, "rewrite_each on empty list")
{
    TestNullRewriter rewriter;
    auto input_ir = gta3sc::LinkedIR<IR>{};
    rewriter.rewrite_each(input_ir);
    CHECK(input_ir.empty());
}

TEST_CASE_FIXTURE(RewriterFixture,
                  "rewrite with multiple replacements splices all and returns "
                  "first")
{
    TestExpandRewriter rewriter(&arena);
    auto input_ir = gta3sc::LinkedIR<IR>{cmd("WAIT"), cmd("RETURN")};

    auto it = rewriter.rewrite(input_ir, input_ir.begin());

    REQUIRE(input_ir
            == gta3sc::LinkedIR<IR>{cmd("NOP"), cmd("GOTO"), cmd("RETURN")});
    REQUIRE((*it).command().name() == "NOP");
}

TEST_CASE_FIXTURE(RewriterFixture,
                  "rewrite_each with multiple replacements backtracks "
                  "correctly")
{
    TestExpandRewriter rewriter(&arena);
    auto input_ir = gta3sc::LinkedIR<IR>{cmd("WAIT"), cmd("RETURN")};

    rewriter.rewrite_each(input_ir);

    CHECK(input_ir
          == gta3sc::LinkedIR<IR>{cmd("NOP"), cmd("GOTO"), cmd("RETURN")});
}

TEST_CASE_FIXTURE(RewriterFixture,
                  "CompositeInstructionRewriter visit chains three rewriters")
{
    auto result
            = gta3sc::CompositeInstructionRewriter{TestReplaceWithNopRewriter(
                                                           &arena),
                                                   TestCommandRewriter(&arena,
                                                                       "NOP",
                                                                       "WAIT"),
                                                   TestCommandRewriter(&arena,
                                                                       "WAIT",
                                                                       "GOTO")}
                      .visit(*cmd("ANYTHING"));

    REQUIRE(result);
    CHECK(*result == gta3sc::LinkedIR<IR>{cmd("GOTO")});
}

TEST_CASE_FIXTURE(
        RewriterFixture,
        "CompositeInstructionRewriter visit stops chain when result is empty")
{
    auto result
            = gta3sc::CompositeInstructionRewriter{TestReplaceWithNopRewriter(
                                                           &arena),
                                                   TestDeleteRewriter{},
                                                   TestCommandRewriter(&arena,
                                                                       "NOP",
                                                                       "GOTO")}
                      .visit(*cmd("ANYTHING"));

    REQUIRE(result);
    CHECK(result->empty());
}

TEST_CASE_FIXTURE(RewriterFixture, "CompositeInstructionRewriter rewrite_each")
{
    auto input_ir = gta3sc::LinkedIR<IR>{cmd("WAIT"), cmd("GOTO")};

    gta3sc::CompositeInstructionRewriter{
            TestCommandRewriter(&arena, "WAIT", "NOP"),
            TestCommandRewriter(&arena, "GOTO", "RETURN")}
            .rewrite_each(input_ir);

    CHECK(input_ir == gta3sc::LinkedIR<IR>{cmd("NOP"), cmd("RETURN")});
}