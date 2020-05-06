#pragma once
#include <gta3sc/command-table.hpp>
#include <gta3sc/diagnostics.hpp>
#include <gta3sc/ir/linked-ir.hpp>
#include <gta3sc/ir/parser-ir.hpp>
#include <gta3sc/ir/sema-ir.hpp>
#include <gta3sc/ir/symbol-table.hpp>
#include <gta3sc/model-table.hpp>
#include <gta3sc/util/arena.hpp>

namespace gta3sc::syntax
{
/// This is a semantic analyzer.
///
/// It receives the intermediate representation of the parser as input, checks
/// whether it is semantically valid, and if so, outputs a intermediate
/// representation guaranted to be semantically valid. It also discovers and
/// inserts names into a symbol repository along the process.
///
/// The output IR is as close as possible to the input IR. That is, no
/// rewriting whatsoever is performed on the arguments (e.g. the argument to
/// SET_PROGRESS_TOTAL is the same as in the input IR, which is guaranted to
/// be zero if semantic checks were successful).
class Sema
{
public:
    /// \param parser_ir the IR to be validated.
    /// \param symrepo the repository that symbols should be inserted in.
    /// \param cmdman a container of command definitions.
    /// \param diag a handler to emit diagnostics into.
    /// \param arena the arena that should be used to allocate IR in.
    explicit Sema(LinkedIR<ParserIR> parser_ir, SymbolTable& symrepo,
                  const CommandTable& cmdman, const ModelTable& modelman,
                  DiagnosticHandler& diag, ArenaAllocator<> allocator) noexcept
        :
        parser_ir(std::move(parser_ir)),
        cmdman(&cmdman),
        modelman(&modelman),
        symrepo(&symrepo),
        diag(&diag),
        allocator(allocator)
    {}

    Sema(const Sema&) = delete;
    auto operator=(const Sema&) -> Sema& = delete;

    Sema(Sema&&) noexcept = default;
    auto operator=(Sema&&) noexcept -> Sema& = default;

    ~Sema() noexcept = default;

    /// Checks the semantic validity of the input IR.
    ///
    /// Returns a semantically valid IR if the input is valid,
    /// otherwise `std::nullopt`.
    auto validate() -> std::optional<LinkedIR<SemaIR>>;

    /// Gets the entity type of the given variable at the end of the input.
    ///
    /// Must run `validate` beforehand.
    [[nodiscard]] auto var_entity_type(const SymbolTable::Variable& var) const
            -> CommandTable::EntityId;

private:
    struct VarRef;

    /// Performs a pass in the input IR to identify declarations
    /// and inserts their names and metadata into the symbol repository.
    auto discover_declarations_pass() -> bool;

    /// Performs a pass in the input IR to check whether it is valid.
    auto check_semantics_pass() -> std::optional<LinkedIR<SemaIR>>;

    // The following methods checks the semantic validity a specific portion
    // of the input IR and tries to produces a output IR.
    //
    // Do note these functions do its best to recover from bad situations,
    // as such it may produce output even for invalid input, although
    // diagnostics are produced for such inputs.
    //
    // Thus, to effectively check whether the portion of input IR is valid,
    // one must compare the value of `report_count` before and after the
    // function call.
    //
    // These functions are called from the semantic checking pass.

    auto validate_label_def(const ParserIR::LabelDef& label_def)
            -> const SymbolTable::Label*;

    auto validate_command(const ParserIR::Command& command)
            -> ArenaPtr<const SemaIR::Command>;

    auto validate_argument(const CommandTable::ParamDef& param,
                           const ParserIR::Argument& arg)
            -> ArenaPtr<const SemaIR::Argument>;

    auto validate_integer_literal(const CommandTable::ParamDef& param,
                                  const ParserIR::Argument& arg)
            -> ArenaPtr<const SemaIR::Argument>;

    auto validate_float_literal(const CommandTable::ParamDef& param,
                                const ParserIR::Argument& arg)
            -> ArenaPtr<const SemaIR::Argument>;

    auto validate_text_label(const CommandTable::ParamDef& param,
                             const ParserIR::Argument& arg)
            -> ArenaPtr<const SemaIR::Argument>;

    auto validate_label(const CommandTable::ParamDef& param,
                        const ParserIR::Argument& arg)
            -> ArenaPtr<const SemaIR::Argument>;

    auto validate_string_literal(const CommandTable::ParamDef& param,
                                 const ParserIR::Argument& arg)
            -> ArenaPtr<const SemaIR::Argument>;

    auto validate_var_ref(const CommandTable::ParamDef& param,
                          const ParserIR::Argument& arg)
            -> ArenaPtr<const SemaIR::Argument>;

    // The following functions validates the semantics of commands that
    // require further compiler intervention. They produce diagnostics
    // and return false in case of ill-formed programs.

    auto validate_hardcoded_command(const SemaIR::Command& command) -> bool;

    auto validate_set(const SemaIR::Command& command) -> bool;

    auto validate_script_name(const SemaIR::Command& command) -> bool;

    auto validate_start_new_script(const SemaIR::Command& command) -> bool;

    auto validate_target_scope_vars(SemaIR::ArgumentView::iterator begin,
                                    SemaIR::ArgumentView::iterator end,
                                    SymbolTable::ScopeId target_scope_id)
            -> bool;

    // The following functions validates declarations and inserts their
    // names and metadata into the symbol repository.
    //
    // Again, these do their best to perform recovery, producing diagnostics
    // along the way, so to check for success one must compare `report_count`
    // before and after the call.
    //
    // These functions are called from the declaration discovery pass.

    void declare_label(const ParserIR::LabelDef& label_def);

    void declare_variable(const ParserIR::Command& command,
                          SymbolTable::ScopeId scope_id,
                          SymbolTable::VarType type);

    /// This function parses a variable reference (encoded as an identifier).
    ///
    /// In case of failure, it produces diagnostics and recovers. As such,
    /// this function never fails, but may produce outputs that do not
    /// coincide to what the user actually wrote in the source code.
    ///
    /// Compare `report_count` before and after the call to check success.
    auto parse_var_ref(std::string_view identifier, SourceRange source)
            -> VarRef;

    /// Reports an invalid situation and increments `report_count`.
    auto report(SourceLocation source, Diag message) -> DiagnosticBuilder;

    /// Reports an invalid situation and increments `report_count`.
    auto report(SourceRange source, Diag message) -> DiagnosticBuilder;

    /// Lookups a variable in the global scope as well as in
    /// the current local scope.
    [[nodiscard]] auto lookup_var_lvar(std::string_view name) const
            -> const SymbolTable::Variable*;

    /// Sets the entity type for `var` in `vars_entity_type`.
    void set_var_entity_type(const SymbolTable::Variable& var,
                             CommandTable::EntityId entity_type);

    /// Finds a constant in the default model enumeration.
    [[nodiscard]] auto find_defaultmodel_constant(std::string_view name) const
            -> const CommandTable::ConstantDef*;

    /// Checks whether the given parameter has an object string constant
    /// association.
    [[nodiscard]] auto
    is_object_param(const CommandTable::ParamDef& param) const -> bool;

    /// Checks whether a parameter type accepts only global variables.
    [[nodiscard]] auto is_gvar_param(CommandTable::ParamType param_type) const
            -> bool;

    /// Checks whether a parameter type accepts only local variables.
    [[nodiscard]] auto is_lvar_param(CommandTable::ParamType param_type) const
            -> bool;

    /// Checks whether the typing of a parameter matches the
    /// typing of a variable.
    [[nodiscard]] auto matches_var_type(CommandTable::ParamType param_type,
                                        SymbolTable::VarType var_type) const
            -> bool;

    /// Checks whether the specified command is an alternative of an certain
    /// alternator.
    [[nodiscard]] auto
    is_alternative_command(const CommandTable::CommandDef& command_def,
                           const CommandTable::AlternatorDef& from) const
            -> bool;

    //// Checks whether the specified actual command/args matches the
    //// specification of a certain alternative command.
    auto is_matching_alternative(const ParserIR::Command& command,
                                 const CommandTable::CommandDef& alternative)
            -> bool;

private:
    struct VarSubscript
    {
        std::string_view
                name; ///< Either a integer or variable name in the subscript.
        SourceRange source; ///< The range of the subscript..
        std::optional<int32_t>
                literal; ///< The integer literal in the subscript if any.
    };

    struct VarRef
    {
        std::string_view name; ///< The name of the variable.
        SourceRange source;    ///< The range of the name of the variable.
        std::optional<VarSubscript> subscript; ///< The subscript if any.
    };

private:
    ArenaAllocator<> allocator;
    DiagnosticHandler* diag; // Do not use directly. Please call `report`.
    SymbolTable* symrepo;
    const CommandTable* cmdman;
    const ModelTable* modelman;
    LinkedIR<ParserIR> parser_ir;

    /// Used to represent no scope for `first_scope` and `current_scope`.
    static constexpr auto no_local_scope = SymbolTable::invalid_scope;

    // The following variables are part of the "state machine" of
    // the semantic checker.
    uint32_t report_count = 0; ///< The number of errors encountered so far.
    SymbolTable::ScopeId first_scope{
            no_local_scope}; ///< The id of the first allocated scope.
    SymbolTable::ScopeId current_scope{
            no_local_scope};   ///< The id of the current scope.
    bool ran_analysis = false; ///< Whether we already ran analysis.
    bool analyzing_var_decl{}; ///< Whether we are checking a var decl.
    bool analyzing_alternative_command{}; ///< Whether we are checking a
                                          ///< command that was found by
                                          ///< matching an alternator.
    bool analyzing_repeat_command{};      ///< Whether we are checking REPEAT.

    const CommandTable::AlternatorDef* alternator_set{};
    const CommandTable::CommandDef* command_script_name{};
    const CommandTable::CommandDef* command_start_new_script{};

    std::optional<CommandTable::EnumId> model_enum;
    std::optional<CommandTable::EnumId> defaultmodel_enum;

    /// Set of script names already seen.
    std::vector<std::string_view> seen_script_names;

    /// The entity type for the variables in the program.
    std::vector<std::vector<CommandTable::EntityId>> vars_entity_type;
};
} // namespace gta3sc::syntax
