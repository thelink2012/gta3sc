///
/// Tokenizer and Parser
///
/// Tokenizer
///     The tokenizer is responsible for transforming a stream of characters into a stream of tokens, which, then,
///     may be easily parsed by the parser.
///
/// Parser
///     The parser is responsible for transforming the stream of tokens into an abstract syntax tree. Such tree,
///     can then be easily parsed by the semantic analyzer in the next compilation step.
///
#pragma once
#include <stdinc.h>

struct ParserContext;

enum class Token
{
    Command,
    Label,
    ScopeBegin,
    ScopeEnd,
    MISSION_START,
    MISSION_END,
    SCRIPT_START,
    SCRIPT_END,
    NewLine,

    VAR_INT,
    VAR_FLOAT,
    VAR_TEXT_LABEL,
    VAR_TEXT_LABEL16,
    LVAR_INT,
    LVAR_FLOAT,
    LVAR_TEXT_LABEL,
    LVAR_TEXT_LABEL16,

    CONST_INT,
    CONST_FLOAT,

    Integer,
    Float,
    Text,
    String,

    NOT,
    AND,
    OR,
    
    IF,
    IFNOT,
    ELSE,
    ENDIF,

    WHILE,
    WHILENOT,
    ENDWHILE,

    REPEAT,
    ENDREPEAT,

    SWITCH,
    ENDSWITCH,
    CASE,
    DEFAULT,
    BREAK,
    CONTINUE,  // Extension

    Decrement,
    Increment,
    Equal,
    Greater,
    GreaterEqual,
    Lesser,
    LesserEqual,
    Plus,
    Minus,
    Times,
    Divide,
    TimedPlus,
    TimedMinus,
    EqCast,
    EqPlus,
    EqMinus,
    EqTimes,
    EqDivide,
    EqTimedPlus,
    EqTimedMinus,

    // Extensions
    Hexadecimal,
    DUMP,
    ENDDUMP,
};

enum class NodeType
{
    Block,
    Command,
    Label,
    Scope,
    MISSION_START,
    MISSION_END,
    SCRIPT_START,
    SCRIPT_END,

    VAR_INT,
    VAR_FLOAT,
    VAR_TEXT_LABEL,
    VAR_TEXT_LABEL16,
    LVAR_INT,
    LVAR_FLOAT,
    LVAR_TEXT_LABEL,
    LVAR_TEXT_LABEL16,
    
    CONST_INT,
    CONST_FLOAT,

    Integer,
    Float,
    Text,
    String,

    NOT,
    AND,
    OR,

    IF,
    IFNOT,
    ELSE,
    WHILE,
    WHILENOT,
    REPEAT,
    SWITCH,
    CASE,
    DEFAULT,
    BREAK,
    CONTINUE, // Extension

    Decrement,
    Increment,
    Cast,
    Equal,
    Greater,
    GreaterEqual,
    Lesser,
    LesserEqual,
    Add,
    Sub,
    Times,
    Divide,
    TimedAdd,
    TimedSub,

    // Extensions
    DUMP,
};

/// A Miss2 Identifier is anything that starts with A-Z or $.
///
/// An array access is in fact a single identifier (i.e. array[1]), as such
/// this little structure is provided to help separating the two pieces.
///
/// This is supposed to be used in the semantic analyzer, the parser only works with full identifier.
struct Miss2Identifier
{
    enum Error
    {
        InvalidIdentifier,
        NestingOfArrays,
        NegativeIndex,
        OutOfRange,
    };

    string_view                             identifier;
    optional<variant<size_t, string_view>>  index;

    /// Matches a miss2 identifier.
    ///
    /// \warning the lifetime of the returned `Miss2Identifier` must be as long
    /// \warning as the lifetime of the view `value`.
    static auto match(const string_view& value, const Options&) -> expected<Miss2Identifier, Error>;

    /// Checks whether a string is a miss2 identifier.
    static bool is_identifier(const string_view& value, const Options& options);
};

inline const char* to_string(Miss2Identifier::Error e)
{
    switch(e)
    {
        case Miss2Identifier::InvalidIdentifier:return "invalid identifier";
        case Miss2Identifier::NestingOfArrays:  return "nesting of arrays not allowed";
        case Miss2Identifier::NegativeIndex:    return "index cannot be negative";
        case Miss2Identifier::OutOfRange:       return "index out of range";
        default:                                Unreachable();
    }
}

optional<int32_t> to_integer(const SyntaxTree&, ProgramContext&);
optional<float> to_float(const SyntaxTree&, ProgramContext&);

///////////////////////////////

class TokenStream : public std::enable_shared_from_this<TokenStream>
{
public:
    struct TokenData
    {
        Token  type;    //< Type of token
        size_t begin;   //< Offset for token in TokenStream::data
        size_t end;     //< Offset for token in TokenStream::data (end)
    };

    struct TextStream
    {
        const std::string   stream_name;  //< Name of this stream (usually name of the source file).
        const std::string   data;         //< UTF-8 source file.
        std::vector<size_t> line_offset;
        size_t              max_offset = 0;

        explicit TextStream(std::string data, std::string name);

        /// Gets the byte offset in this->text() that the specified line number (1-based) is in.
        ///
        /// \throws std::logic_error if lineno does not exist.
        size_t offset_for_line(size_t lineno) const;

        /// Gets the content of the specified line number (1-based).
        ///
        /// \throws std::logic_error if lineno does not exist.
        std::string get_line(size_t lineno) const;

        /// Gets the (lineno, colno) (1-based) of the specified offset in the stream data.
        ///
        /// \throws std::logic_error if offset is out of range.
        std::pair<size_t, size_t> linecol_from_offset(size_t offset) const;

        /// Gets the text in the stream in the specified range.
        string_view get_text(size_t begin, size_t end) const; 
    };

    // Used for error messages.
    struct TokenInfo
    {
        const TextStream& stream;
        size_t            begin;
        size_t            end;

        explicit TokenInfo(const TextStream& stream, size_t begin, size_t end)
            : stream(stream), begin(begin), end(end)
        {}

        explicit TokenInfo(const TextStream& stream, const TokenData& token)
            : TokenInfo(stream, token.begin, token.end)
        {}
    };

    const TextStream              text;     //< Source file.
    const std::vector<TokenData>  tokens;   //< Tokenized source file.

public:
    /// Tokenizes the specified file.
    static std::shared_ptr<TokenStream> tokenize(ProgramContext&, const fs::path&);

    /// Tokenizes the specified data.
    static std::shared_ptr<TokenStream> tokenize(ProgramContext&, std::string data, const char* stream_name);

    TokenStream(TokenStream&&);
    TokenStream(const TokenStream&) = delete;

    /// For debugging purposes.
    std::string to_string() const;

private:
    ProgramContext&         program;

    explicit TokenStream(ProgramContext&, const char* stream_name, std::string data, std::vector<TokenData>);
    explicit TokenStream(ProgramContext&, TextStream stream, std::vector<TokenData>);
};

///////////////////////////////

class SyntaxTree : public std::enable_shared_from_this<SyntaxTree>
{
public:
    using iterator       = std::vector<std::shared_ptr<SyntaxTree>>::iterator;
    using const_iterator = std::vector<std::shared_ptr<SyntaxTree>>::const_iterator;

public:
    static std::shared_ptr<SyntaxTree> compile(ProgramContext&, const TokenStream& tstream);
    SyntaxTree(const SyntaxTree&) = delete;
    SyntaxTree(SyntaxTree&&);
    
    /// Gets the type of this node.
    NodeType type() const
    {
        return this->type_;
    }

    /// Text associated with the token in this node.
    string_view text() const
    {
        Expects(this->instream != nullptr);
        auto source_data = this->instream->tstream.lock()->text.data.c_str();
        return string_view(source_data + this->token.begin, this->token.end - this->token.begin);
    }

    /// Checks if `text().empty()`.
    bool has_text() const
    {
        if(this->instream)
            return (this->token.begin != this->token.end);
        return false;
    }

    /// Iterator to childs (begin).
    iterator begin()
    {
        return this->childs.begin();
    }

    /// Iterator to childs (end).
    iterator end()
    {
        return this->childs.end();
    }

    /// Iterator to childs (begin).
    const_iterator begin() const
    {
        return this->childs.begin();
    }

    /// Iterator to childs (end).
    const_iterator end() const
    {
        return this->childs.end();
    }

    /// Number of childs on this node.
    size_t child_count() const
    {
        return this->childs.size();
    }
    
    /// Gets the child at the specified index.
    const SyntaxTree& child(size_t i) const
    {
        return *this->childs[i];
    }

    /// Gets the child at the specified index.
    SyntaxTree& child(size_t i)
    {
        return *this->childs[i];
    }

    /// Gets the parent node, or `nullptr` if none.
    std::shared_ptr<SyntaxTree> parent() const
    {
        if(this->parent_)
            return this->parent_.value().lock();
        return nullptr;
    }

    // Adds a child to this node.
    void add_child(shared_ptr<SyntaxTree> child)
    {
        Expects(child->parent_ == nullopt);
        child->parent_ = std::weak_ptr<SyntaxTree>(this->shared_from_this());
        this->childs.emplace_back(std::move(child));
    }

    // Steals the childs from the other tree.
    void take_childs(shared_ptr<SyntaxTree>& other)
    {
        this->childs.reserve(this->childs.size() + other->childs.size());
        for(auto& child : other->childs)
        {
            child->parent_ = std::weak_ptr<SyntaxTree>(this->shared_from_this());
            this->childs.emplace_back(std::move(child));
        }

        other->childs.clear();
    }

    /// Performs a pre-ordered depth-first traversal on this tree.
    ///
    /// Does not go any deeper in a node that `fun()` returns false.
    template<typename Functor>  // Functor = bool(SyntaxTree)
    void depth_first(Functor fun) //const
    {
        if(fun(*this))
        {
            for(auto& child : *this)
                child->depth_first(std::ref(fun));
        }
    }

    /// Sets the annotation for this node.
    template<typename ValueType>
    void set_annotation(ValueType&& v)
    {
        this->udata = std::forward<ValueType>(v);
    }

    /// Gets the annotation of this node, previosly set with `set_annotation`.
    ///
    /// Note: You can get a ref by using e.g. `<int&>` instead of `<int>`.
    ///
    /// \throws bad_any_cast if there's no annotation on this node.
    template<typename T>
    T annotation() const
    {
        return any_cast<T>(this->udata);
    }

    /// Gets the annotation of this node, previosly set with `set_annotation`, or `nullopt` if not set.
    ///
    /// Note: You can get a ref by using e.g. `<int&>` instead of `<int>`.
    template<typename T>
    optional<T> maybe_annotation() const
    {
        using TNoRef = std::remove_reference_t<T>;
        if(const TNoRef* p = any_cast<TNoRef>(&this->udata))
            return *p;
        return nullopt;
    }

    /// Checks if this node has been annotated.
    bool is_annotated() const
    {
        return !this->udata.empty();
    }

    /// Returns an `any` object associated with the annotation.
    const any& annotation_any() const
    {
        return this->udata;
    }

    /// Filename of the input stream associated with this SyntaxTree, or empty if none.
    std::string filename() const
    {
        return this->instream? *this->instream->filename : "";
    }

    /// Input stream associated with this SyntaxTree, or `nullptr` if none.
    weak_ptr<const TokenStream> token_stream() const
    {
        return this->instream? this->instream->tstream : weak_ptr<const TokenStream>();
    }

    ///
    const TokenStream::TokenData get_token() const
    {
        Expects(this->instream != nullptr);
        return this->token;
    }

    /// For debugging purposes.
    std::string to_string(size_t level = 0) const;

protected:
    friend class TokenStream;
    friend struct ParserContext;

    struct InputStream
    {
        shared_ptr<std::string>     filename;   //< Name of the input file. Stored also here because tstream may get deallocated.
        weak_ptr<const TokenStream> tstream;    //< Input token stream, if still allocated.
    };

private:
    NodeType                                    type_;  // const NodeType
    TokenStream::TokenData                      token;      // invalid if (instream == nullptr)
    shared_ptr<InputStream>                     instream;   // may be nullptr
    std::vector<std::shared_ptr<SyntaxTree>>    childs;
    optional<std::weak_ptr<SyntaxTree>>         parent_;
    any                                         udata;

public:
    explicit SyntaxTree(NodeType type, any udata)
        : type_(type), instream(nullptr), udata(std::move(udata))
    {
    }

    explicit SyntaxTree(NodeType type, shared_ptr<InputStream>& instream, const TokenStream::TokenData& token)
        : type_(type), instream(instream), token(token)
    {
    }

    explicit SyntaxTree(NodeType type)
        : type_(type), instream(nullptr)
    {
    }

    shared_ptr<SyntaxTree> clone() const;
};
