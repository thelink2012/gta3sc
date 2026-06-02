///
/// Compiler
///
/// The compiler is responsible for transforming a Annotated Syntax Tree, plus a previosly generated Symbol Table of such tree,
/// into a intermediate representation (vector of pseudo-instructions).
///
/// This representation will **later** on be used by the code generator (codegen.hpp/cpp) to build the SCM Bytecode.
///
#pragma once
#include <stdinc.h>
#include "program.hpp"

/// IR for variable / array.
struct CompiledVar
{
    shared_ptr<Var>                             var;
    optional<variant<int32_t, shared_ptr<Var>>> index;

    explicit CompiledVar(shared_ptr<Var> var, nullopt_t)
        : var(std::move(var)), index(nullopt)
    {}

    explicit CompiledVar(shared_ptr<Var> var, int32_t index)
        : var(std::move(var)), index(index)
    {}

    explicit CompiledVar(shared_ptr<Var> var, shared_ptr<Var> index)
        : var(std::move(var)), index(std::move(index))
    {}

    bool operator==(const CompiledVar& rhs) const
    {
        return this->var == rhs.var && this->index == rhs.index;
    }
};

/// IR for strings, no matter if it's a fixed size (8/16/128 bytes) or var length.
struct CompiledString
{
    enum class Type : uint8_t
    {
        TextLabel8,
        TextLabel16,
        String128,
        StringVar,
    };

    Type        type;
    bool        preserve_case;
    std::string storage;
};

/// IR for a single argument of a command.
using ArgVariant = variant<EOAL, int8_t, int16_t, int32_t, float, shared_ptr<Label>, CompiledVar, CompiledString>;

/// IR for a single command plus its arguments.
struct CompiledCommand
{
    bool                    not_flag;
    const Command&          command;
    std::vector<ArgVariant> args;
};

/// IR for label **definitions**.
///
/// This is just a helper to find out where the labels are.
struct CompiledLabelDef
{
    shared_ptr<Label> label;

    size_t compiled_size() const
    {
        return 0;
    }
};

/// IR for HEX data.
struct CompiledHex
{
    std::vector<uint8_t> data;

    size_t compiled_size() const
    {
        return data.size();
    }
};

// IR for SCM header
struct CompiledScmHeader
{
    enum class Version : uint8_t
    {
        Liberty,
        Miami,
        SanAndreas,
    };

    Version                               version;                  //< Version of the header.
    uint32_t                              size_global_vars_space;   //< (includes the 8 bytes of GOTO at the top)
    std::vector<std::string>              models;                   //< Models header.
    std::vector<shared_ptr<const Script>> base_scripts;             //< All non-require scripts being compiled into the multifile/script.img.
    uint32_t                              num_missions;             //< Number of missions.
    uint32_t                              num_streamed;             //< Number of streamed scripts.

    explicit CompiledScmHeader(Version version, size_t size_globals,
                               std::vector<std::string> models_,
                               const std::vector<shared_ptr<Script>>& scripts) :
        version(version),
        size_global_vars_space(std::max(size_t(8), size_globals)),
        models(std::move(models_)),
        num_missions(0), num_streamed(0)
    {
        this->base_scripts.reserve(scripts.size());
        for(auto& sc : scripts)
        {
            assert(!sc->is_child_of_custom());
            if(sc->type == ScriptType::Required)
                continue;

            this->base_scripts.emplace_back(sc);

            if(sc->type == ScriptType::Mission)
                ++this->num_missions;
            else if(sc->type == ScriptType::StreamedScript)
                ++this->num_streamed;
        }
    }

    size_t compiled_size() const;
};

/// IR for a fundamental piece of compiled data. May be a label or a command.
struct CompiledData
{
    variant<CompiledLabelDef, CompiledCommand, CompiledHex> data;

    CompiledData(CompiledCommand x)
        : data(std::move(x))
    {}

    CompiledData(std::vector<uint8_t> x)
        : data(CompiledHex { std::move(x) })
    {}

    CompiledData(shared_ptr<Label> x)
        : data(CompiledLabelDef{ std::move(x) })
    {}
};

/// Transforms an annotated syntax tree into a intermediate representation (vector of pseudo-instructions).
class CompilerContext
{
private:
    struct LoopInfo
    {
        shared_ptr<Label> continue_label;   //< Where a CONTINUE should jump into (may be nullptr).
        shared_ptr<Label> break_label;      //< Where a BREAK should jump into
    };

    // Helpers
    shared_ptr<Scope>              current_scope;
    std::vector<shared_ptr<Label>> internal_labels;
    std::vector<LoopInfo>          loop_stack;
    shared_ptr<Label>              label_skip_cutscene_end;

    // Inputs
    ProgramContext&                 program;
    const Commands&                 commands;
    
    // Output
    std::vector<CompiledData>       compiled;

public:
    // Inputs
    const shared_ptr<const Script>  script;
    const SymTable&                 symbols;

public:
    explicit CompilerContext(shared_ptr<const Script> script, const SymTable& symbols, ProgramContext& program)
        : script(std::move(script)), symbols(symbols), commands(program.commands), program(program)
    {
        this->loop_stack.reserve(16);
    }

    static auto compile(shared_ptr<const Script> script, const SymTable& symbols, ProgramContext& program) -> CompilerContext
    {
        CompilerContext c(std::move(script), symbols, program);
        c.compile();
        return c;
    }

    /// Compiles everything on the Syntax Tree of the script.
    void compile();

    /// Gets the result of `compile`.
    const std::vector<CompiledData>& get_data() const& { return this->compiled; }
    std::vector<CompiledData>& get_data() &            { return this->compiled; }
    std::vector<CompiledData> get_data() &&            { return std::move(this->compiled); }

private:

    using ArgList = std::vector<ArgVariant>;

    struct Case;
    struct LoopInfo;

    shared_ptr<Label> make_internal_label();

    void compile_statements(const SyntaxTree& parent, size_t from_id, size_t to_id_including);

    void compile_statements(const SyntaxTree& base);

    void compile_statement(const SyntaxTree& node, bool not_flag = false);

    void compile_label(const SyntaxTree& label_node);

    void compile_label(shared_ptr<Label> label_ptr);

    void compile_command(const Command& command, ArgList args, bool not_flag = false);

    void compile_command(const SyntaxTree& command_node, bool not_flag = false);

    void compile_scope(const SyntaxTree& scope_node);

    void compile_if(const SyntaxTree& if_node, bool not_flag);

    void compile_while(const SyntaxTree& while_node, bool not_flag);

    void compile_repeat(const SyntaxTree& repeat_node);

    void compile_switch(const SyntaxTree& switch_node);

    // \warning mutates `cases`.
    // \warning expects no repeated Cases.
    void compile_switch_withop(const SyntaxTree& swnode, std::vector<Case>& cases, shared_ptr<Label> break_ptr);

    void compile_switch_ifchain(const SyntaxTree& swnode, std::vector<Case>& cases, shared_ptr<Label> break_ptr);

    void compile_break(const SyntaxTree& break_node);

    void compile_continue(const SyntaxTree& continue_node);

    void compile_expr(const SyntaxTree& eq_node, bool not_flag = false);

    void compile_incdec(const SyntaxTree& op_node, bool not_flag = false);

    void compile_mission_end(const SyntaxTree& me_node, bool not_flag = false);

    void compile_condition(const SyntaxTree& node, bool not_flag = false);

    void compile_conditions(const SyntaxTree& conds_node, const shared_ptr<Label>& else_ptr, bool not_flag);

    void compile_dump(const SyntaxTree& node);

private:

    ArgList get_args(const Command& command, const SyntaxTree& command_node);

    ArgList get_args(const Command& command, const std::vector<any>& params);

    ArgVariant get_arg(const Commands::MatchArgument& a);

    ArgVariant get_arg(const SyntaxTree& arg_node);

    ArgVariant get_arg(const any& param);

    bool is_same_var(const ArgVariant& lhs, const ArgVariant& rhs);

private:
    /// Helper for the SWITCH statement.
    struct Case
    {
        optional<int32_t>            value;
        shared_ptr<Label>            target;
        optional<const Command*>     is_var_eq_int;
        size_t                       first_statement_id = SIZE_MAX;
        size_t                       last_statement_id = SIZE_MAX;

        explicit Case(optional<int32_t> value, optional<const Command*> veqi) :
            value(std::move(value)), is_var_eq_int(std::move(veqi))
        {}

        bool is_default() const
        {
            return this->value == nullopt;
        }

        bool is_empty() const
        {
            return this->first_statement_id == SIZE_MAX;
        }

        bool same_body_as(const Case& rhs) const
        {
            Expects(this->first_statement_id != SIZE_MAX);
            Expects(rhs.first_statement_id != SIZE_MAX);
            return this->first_statement_id == rhs.first_statement_id;
        }
    };

};
