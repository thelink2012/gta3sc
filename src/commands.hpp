#pragma once
#include <stdinc.h>

/// Fundamental type of a command argument.
enum class ArgType : uint8_t
{
    Param,
    Label,
    Integer,
    Float,
    TextLabel,
    TextLabel16,
    TextLabel32,
    String,
    Constant,
};

///
using EntityType = uint16_t;

/// Stores constant values associated with a identifiers.
struct Enum
{
    insensitive_map<std::string, int32_t> values;
    bool is_global = false;

    explicit Enum(insensitive_map<std::string, int32_t> values, bool is_global) :
        values(std::move(values)), is_global(is_global)
    {}

    optional<int32_t> find(const string_view& value) const
    {
        auto it = values.find(value);
        if(it != values.end())
            return it->second;
        return nullopt;
    }
};

/// Stores command information.
struct Command
{
    struct Arg
    {
        ArgType type;               //< Fundamental type of argument.
        bool optional : 1;          //< Allows as many optional arguments as possible (must be the last arg).
        bool is_output : 1;         //< Whether this argument outputs values into a variable.
        bool is_ref : 1;            //< Whether this argument is taken by reference by the script engine.
        bool allow_constant : 1;    //< Allow literal values
        bool allow_global_var : 1;  //< Allow global variables
        bool allow_local_var : 1;   //< Allow local variables
        bool allow_text_label : 1;  //< Allow text labels (and its variables) [valid for PARAM arguments only].
        bool allow_pointer : 1;     //< Allow INT values in STRING arguments.
        bool preserve_case : 1;     //< Preserves the case of a string literal.
        EntityType entity_type;     ///< Entity type of this argument. Zero means none.
        std::vector<shared_ptr<Enum>> enums;
        
        explicit Arg()
        {}

        explicit Arg(ArgType type) :
            type(type), optional(false),
            is_output(false), allow_constant(true), allow_global_var(true), allow_local_var(true),
            preserve_case(false), allow_pointer(false), allow_text_label(false)
        {
        }

        /// Finds a constant associated with the enums of this argument.
        ::optional<int32_t> find_constant(const string_view& value) const
        {
            for(auto& e : enums)
            {
                if(auto opt = e->find(value))
                    return opt;
            }
            return nullopt;
        }

        /// Checks whether this argument uses the specified enum.
        bool uses_enum(const shared_ptr<Enum>& e) const
        {
            if(!e) return false;
            return std::find(enums.begin(), enums.end(), e) != enums.end();
        }
    };

    bool                    supported;  //< Is this command supported by the script engine?
    bool                    internal;   //< Is this a command handled solely by the compiler itself?
    bool                    extension;  //< Is this a language extension?
    optional<uint16_t>      id;         //< The opcode id.
    optional<uint32_t>      hash;       //< The command hash.
    small_vector<Arg, 12>   args;       //< The arguments of the command.
    const std::string       name;       //< The name of this command.

    /// Checks if there's any optional argument on this command.
    bool has_optional() const
    {
        return args.empty()? false : args.back().optional;
    }

    /// Gets the argument for the argument position `pos`.
    /// If this argument contains optional arguments, keeps returning them the `pos` of the usual arguments.
    optional<const Arg&> arg(size_t pos) const
    {
        if(pos >= this->args.size())
        {
            if(this->has_optional())
                return this->args.back();
            return nullopt;
        }
        else
        {
            return this->args[pos];
        }
    }

    /// The minimum number of arguments for this command.
    size_t minimum_args() const
    {
        if(this->has_optional())
            return this->args.size() - 1;
        return this->args.size();
    }

    friend bool operator<(const Command& lhs, const Command& rhs)
    {
        return iless()(lhs.name, rhs.name);
    }

    template<typename StringView>
    friend bool operator<(const Command& lhs, const StringView& rhs)
    {
        return iless()(lhs.name, rhs);
    }

    template<typename StringView>
    friend bool operator<(const StringView& lhs, const Command& rhs)
    {
        return iless()(lhs, rhs.name);
    }
};


/// Stores the list of commands and alternators.
class Commands
{
public:
    using Alternator = std::vector<const Command*>;

    struct MatchFailure
    {
        enum Reason
        {
            UnexpectedFailure,
            NoCommandMatch,
            NoAlternativeMatch,
            TooManyArgs,
            TooFewArgs,
            BadArgument,
            ExpectedInt,
            ExpectedFloat,
            ExpectedTextLabel,
            ExpectedStringLiteral,
            NoSuchLabel,
            NoSuchConstant,
            ExpectedVar,
            NoSuchVar,
            InvalidIdentifier,
            IdentifierIndexNesting,
            IdentifierIndexNegative,
            IdentifierIndexOutOfRange,
            VariableIndexNotInt,
            VariableIndexNotVar,
            VariableIndexIsArray,
            VariableIndexIsConstant,
            VariableIndexOutOfRange,
            VariableKindNotAllowed,
            VariableNotAllowed,
            VariableTypeMismatch,
            VariableIsNotArray,
            StringLiteralNotAllowed,
            ExpectedVarIndex,
            LiteralValueDisallowed,
            UserConstantNotInteger,
            UserConstantNotFloat,
        };

        shared_ptr<const SyntaxTree>  context;
        Reason                        reason;

        void emit(ProgramContext&);
        std::string to_string();
    };

    using MatchArgument = variant<const SyntaxTree*, int32_t, float>;
    using MatchArgumentList = small_vector<MatchArgument, 16>;

    using AnnotateArgument = variant<SyntaxTree*, nullopt_t>;
    using AnnotateArgumentList = small_vector<AnnotateArgument, 16>;

public:
    explicit Commands(transparent_set<Command>&& commands,
                      insensitive_map<std::string, std::vector<const Command*>>&& alternators,
                      transparent_map<std::string, EntityType>&& entities,
                      transparent_map<std::string, shared_ptr<Enum>>&& enums);

    Commands(const Commands&) = delete;
    Commands(Commands&&) = default;

    static Commands from_xml(const std::string& config_name, const std::vector<fs::path>& xml_list);
    // TODO ^ make the paths of xml_list absolute? i.e. move modifies to outside?

    /// Adds the default models associated with the program context into the DEFAULTMODEL enum.
    void add_default_models(const insensitive_map<std::string, uint32_t>&);

    /// Gets the MODEL enumeration.
    const shared_ptr<Enum>& get_models_enum() const { return this->enum_models; }

    /// Gets the DEFAULTMODEL enumeration.
    const shared_ptr<Enum>& get_defaultmodel_enum() const { return this->enum_defaultmodels; }

    /// Gets the SCRIPTSTREAM enumeration.
    const shared_ptr<Enum>& get_scriptstream_enum() const { return this->enum_scriptstream; }

    // Argument matching methods.
    expected<const Command*, MatchFailure> match(const SyntaxTree& cmdnode, const SymTable&, const shared_ptr<Scope>&, const Options&) const;
    expected<const Command*, MatchFailure> match(const Command&, const SyntaxTree& cmdnode, const SymTable&, const shared_ptr<Scope>&, const Options&) const;
    expected<const Command*, MatchFailure> match(const Alternator&, const SyntaxTree& cmdnode, const SymTable&, const shared_ptr<Scope>&, const Options&) const;
    expected<const Command*, MatchFailure> match(const Command&, optional<const SyntaxTree&> cmdnode, const MatchArgumentList& args,
                                                 const SymTable&, const shared_ptr<Scope>&, const Options&) const;
    expected<const Command*, MatchFailure> match(const Alternator&, optional<const SyntaxTree&> cmdnode, const MatchArgumentList& args,
                                                 const SymTable&, const shared_ptr<Scope>&, const Options&) const;

    // Syntax tree annotation methods.
    void annotate(SyntaxTree&, const Command&, const SymTable&, const shared_ptr<Scope>&, Script&, ProgramContext&) const;
    void annotate(const AnnotateArgumentList&, const Command&, const SymTable&, const shared_ptr<Scope>&, Script&, ProgramContext&) const;

    /// Finds the integer value of the string constant `value`.
    ///
    /// The `context_free_only` is whether we only search for constants that can be used in 
    ///any occasion or  constants that can be used only in specific commands arguments.
    optional<int32_t> find_constant(const string_view& value, bool context_free_only) const;

    /// Finds the integer value of a string constant 'value'.
    ///
    /// This version searches for all enums in order to allow CONST type arguments.
    optional<int32_t> find_constant_all(const string_view& value) const;

    /// Finds the integer value of a string constant `value` assuming we're handling the argument `arg`.
    optional<int32_t> find_constant_for_arg(const string_view& value, const Command::Arg& arg) const;

    /// Finds the name of the entity assigned to the id `type`.
    optional<std::string> find_entity_name(EntityType type) const;

    /// Find a command based on its name.
    optional<const Command&> find_command(string_view name) const
    {
        auto it = this->commands.find(name);
        if(it != this->commands.end())
            return *it;
        return nullopt;
    }

    /// Find a command based on its hash.
    optional<const Command&> find_command(uint32_t hash, optional<string_view> name) const
    {
        if(name) return this->find_command(*name);
        // TODO lookup by hash.
        return nullopt;
    }

    /// Finds a alternator based on its name.
    optional<const Alternator&> find_alternator(string_view name) const
    {
        auto it = this->alternators.find(name);
        if(it != this->alternators.end())
            return it->second;
        return nullopt;
    }

    /// Find a command based on its id.
    optional<const Command&> find_command(uint16_t id) const
    {
        auto range = this->commands_by_id.equal_range(id);
        for(auto it = range.first; it != range.second; ++it)
        {
            if(!it->second->extension)
                return *it->second;
        }
        if(range.first != range.second)
            return *range.first->second;
        return nullopt;
    }

    //
    // Commands equality check.
    //

    bool equal(const Command& rhs, const Command& lhs) const
    {
        return &rhs == &lhs;
    }

    bool equal(const Command& rhs, optional<const Command&> lhs) const
    {
        if(lhs) return equal(rhs, *lhs);
        return false;
    }

    bool is_alternator(const Command& command, optional<const Alternator&> alt) const
    {
        if(alt)
        {
            for(auto it = alt->begin(); it != alt->end(); ++it)
            {
                if(this->equal(command, **it))
                    return true;
            }
        }
        return false;
    }

private:
    transparent_set<Command> commands;
    insensitive_map<std::string, std::vector<const Command*>> alternators;
    std::multimap<uint16_t, const Command*> commands_by_id;
    transparent_map<std::string, shared_ptr<Enum>> enums;
    transparent_map<std::string, EntityType> entities;

    shared_ptr<Enum> enum_models;
    shared_ptr<Enum> enum_defaultmodels;
    shared_ptr<Enum> enum_scriptstream;

public:
    optional<const Command&> set_progress_total;
    optional<const Command&> set_total_number_of_missions;
    optional<const Command&> set_collectable1_total;
    optional<const Command&> register_mission_passed;
    optional<const Command&> register_oddjob_mission_passed;
    optional<const Command&> create_collectable1;
    optional<const Command&> player_made_progress;
    optional<const Command&> set_mission_respect_total;
    optional<const Command&> award_player_mission_respect;
    optional<const Command&> repeat;
    optional<const Command&> switch_;
    optional<const Command&> case_;
    optional<const Command&> switch_start;
    optional<const Command&> switch_continued;
    optional<const Command&> gosub_file;
    optional<const Command&> return_;
    optional<const Command&> launch_mission;
    optional<const Command&> load_and_launch_mission_internal;
    optional<const Command&> start_new_script;
    optional<const Command&> start_new_streamed_script;
    optional<const Command&> terminate_this_script;
    optional<const Command&> script_name;
    optional<const Command&> cleo_call;
    optional<const Command&> cleo_return;
    optional<const Command&> terminate_this_custom_script;
    optional<const Command&> goto_;
    optional<const Command&> goto_if_true;
    optional<const Command&> goto_if_false;
    optional<const Command&> andor;
    optional<const Command&> register_streamed_script_internal;
    optional<const Command&> save_string_to_debug_file;
    optional<const Command&> skip_cutscene_start;
    optional<const Command&> skip_cutscene_end;
    optional<const Command&> skip_cutscene_start_internal;
    optional<const Command&> require;
    optional<const Alternator&> set;
    optional<const Alternator&> cset;
    optional<const Alternator&> add_thing_to_thing;
    optional<const Alternator&> sub_thing_from_thing;
    optional<const Alternator&> mult_thing_by_thing;
    optional<const Alternator&> div_thing_by_thing;
    optional<const Alternator&> add_thing_to_thing_timed;
    optional<const Alternator&> sub_thing_from_thing_timed;
    optional<const Alternator&> is_thing_equal_to_thing;
    optional<const Alternator&> is_thing_greater_than_thing;
    optional<const Alternator&> is_thing_greater_or_equal_to_thing;
};
