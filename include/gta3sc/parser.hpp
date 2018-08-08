#include <gta3sc/scanner.hpp>
#include <gta3sc/arena-allocator.hpp>
#include <variant>

#include <algorithm>
#include <cstring>

namespace gta3sc
{
/// This is an intermediate representation for syntactically
/// valid GTA3script.
/// 
/// The implication of this being syntatically valid is that e.g.
/// for every WHILE command, there is a matching ENDWHILE one.
///
/// Structured statements such as IF and WHILE also become IR (they
/// are commands after all) where their first argument determines how
/// many of the following instructions are part of the conditional
/// list, much like the unstructured ANDOR command.
///
/// This IR preserves source code information such as the location of
/// each of its identifiers.
struct ParserIR
{
    struct SourceInfo
    {
        const SourceFile& source;
        SourceRange range;
    };


    struct Identifier { std::string_view name; };

    struct Filename { std::string_view filename; };

    struct String { std::string_view string; };

    struct Argument
    {
        arena_ptr<SourceInfo> source_info;
        std::variant<
            int32_t,
            float,
            Identifier,
            Filename,
            String> value;
    };


    struct LabelDef { std::string_view name; };

    struct Command
    {
        arena_ptr<SourceInfo> source_info;
        std::string_view name;
        arena_ptr<arena_ptr<Argument>> arguments = nullptr;
        size_t num_arguments = 0;
    };

    std::variant<LabelDef, Command> op;
    arena_ptr<ParserIR> next = nullptr;
    arena_ptr<ParserIR> prev = nullptr;

    static auto create_source_info(const SourceInfo& info, ArenaMemoryResource& arena) -> arena_ptr<SourceInfo>
    {
        return new (arena, alignof(SourceInfo)) SourceInfo(info);
    }

    static auto create_command(const SourceInfo& info, std::string_view name_a, ArenaMemoryResource& arena) -> arena_ptr<ParserIR>
    {
        auto node = new (arena, alignof(ParserIR)) ParserIR;
        auto name = create_upper_view(name_a, arena);
        node->op = Command { create_source_info(info, arena), name };
        return node;
    }
                            

    static auto create_integer(const SourceInfo& info, int32_t value_,
                              ArenaMemoryResource& arena) -> arena_ptr<Argument>
    {
        decltype(Argument::value) value = value_;
        arena_ptr<Argument> arg_ptr = new (arena, alignof(Argument)) Argument { create_source_info(info, arena), value };
        return arg_ptr;
    }

    static auto create_float(const SourceInfo& info, float value_,
                           ArenaMemoryResource& arena) -> arena_ptr<Argument>
    {
        auto arg = create_integer(info, 0, arena);
        arg->value = value_;
        return arg;
    }

    static auto create_identifier(const SourceInfo& info,
                                std::string_view name,
                                ArenaMemoryResource& arena) -> arena_ptr<Argument>
    {
        auto arg = create_integer(info, 0, arena);
        arg->value = Identifier { create_upper_view(name, arena) };
        return arg;
    }

    static auto create_string(const SourceInfo& info,
                            std::string_view string,
                            ArenaMemoryResource& arena) -> arena_ptr<Argument>
    {
        auto arg = create_integer(info, 0, arena);
        arg->value = String { create_string_view(string, arena) };
        return arg;
    }

    static auto create_upper_view(std::string_view from, ArenaMemoryResource& arena)
        -> std::string_view
    {
        auto ptr = (char*) arena.allocate(from.size(), alignof(char));
        std::transform(from.begin(), from.end(), ptr, [](unsigned char c) {
            return c >= 'a' && c <= 'z'? (c - 32) : c;
        });
        return std::string_view(ptr, from.size());
    }

    static auto create_string_view(std::string_view from, ArenaMemoryResource& arena)
        -> std::string_view
    {
        auto ptr = (char*) arena.allocate(from.size(), alignof(char));
        std::memcpy(ptr, from.data(), from.size());
        return std::string_view(ptr, from.size());
    }
};

// TODO static_assert trivial destructible everything

class Parser
{
public:
    explicit Parser(Scanner scanner_, ArenaMemoryResource& arena_) :
        scanner(std::move(scanner_)),
        arena(arena_)
    {
        std::fill(std::begin(has_peek_token), std::end(has_peek_token), false);
    }

    /// Gets the source file associated with this parser.
    auto source_file() const -> const SourceFile&
    {
        return scanner.source_file();
    }

    void skip_current_line();

    auto parse_command_statement()
        -> std::optional<arena_ptr<ParserIR>>;

    // TODO remove this
    auto to_scanner() && -> Scanner
    {
        return std::move(scanner);
    }

private:
    auto parse_command(bool is_if_line = false) 
        -> std::optional<arena_ptr<ParserIR>>;

    auto parse_expression()
        -> std::optional<arena_ptr<ParserIR>>;

    auto parse_argument() 
        -> std::optional<arena_ptr<ParserIR::Argument>>;

    bool is_integer(const Token&) const;
    bool is_float(const Token&) const;
    bool is_identifier(const Token&) const;
    bool is_digit(char) const;

private:
    // you can only peek if you peek the previous one
    auto peek(size_t n = 0) -> std::optional<Token>
    {
        assert(n < std::size(peek_tokens));
        if(has_peek_token[n])
            return peek_tokens[n];

        size_t i = 0;
        while(i < size(peek_tokens) && has_peek_token[i])
            ++i;

        if(i > 0 && peek_tokens[i-1] && peek_tokens[i-1]->category == Category::EndOfLine)
            return std::nullopt;

        while(i < n)
        {
            has_peek_token[i] = true;
            peek_tokens[i] = scanner.next();
            ++i;

            if(peek_tokens[i-1]->category == Category::EndOfLine)
                return std::nullopt;
        }

        assert(i == n);
        has_peek_token[i] = true;
        peek_tokens[i] = scanner.next();

        return peek_tokens[n];
    }

    auto peek_category(size_t n = 0) -> std::optional<Category>
    {
        auto token = peek();
        if(!token)
            return std::nullopt;
        return token->category;
    }

    auto next() -> std::optional<Token>
    {
        if(!has_peek_token[0])
            return scanner.next();

        assert(has_peek_token[0] == true);
        auto eat_token = peek_tokens[0];

        size_t n = 1;
        for(; n < size(peek_tokens) && has_peek_token[n]; ++n)
            peek_tokens[n-1] = peek_tokens[n];

        assert(has_peek_token[n-1] == true);
        if(peek_tokens[n-1] == std::nullopt
            || peek_tokens[n-1]->category == Category::EndOfLine)
        {
            has_peek_token[n-1] = false;
        }
        else
        {
            has_peek_token[n-1] = true;
            peek_tokens[n-1] = scanner.next();
        }
        
        return eat_token;
    }

    auto consume() -> std::optional<Token>
    {
        auto token = next();
        if(!token)
        {
            // TODO emit some error derived from scanner
            return std::nullopt;
        }
        return token;
    }

    template<typename... Args>
    auto consume(Args&&... category) -> std::optional<Token>
    {
        // TODO put is same for type category
        auto token = next();
        if(!token)
        {
            // TODO emit some error derived from the scanner
            return std::nullopt;
        }
        if(((token->category != category) && ...))
        {
            // TODO emit expected error
            return std::nullopt;
        }
        return token;
    }

private:
    Scanner scanner;
    ArenaMemoryResource& arena;

    bool has_peek_token[6];
    std::optional<Token> peek_tokens[6];
};
}
