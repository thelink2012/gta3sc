#pragma once
#include <gta3sc/arena-allocator.hpp>
//#include <gta3sc/command-manager.hpp>
#include <gta3sc/ir/parser-ir.hpp>
#include <gta3sc/ir/linked-ir.hpp>
#include <gta3sc/ir/symbol-table.hpp>
#include <gta3sc/diagnostics.hpp>

namespace gta3sc
{
class Sema
{
public:
    explicit Sema(LinkedIR<ParserIR> parser_ir_,
//                  const CommandManager& cmdman_,
                  SymbolRepository& symrepo_,
                  DiagnosticHandler& diag,
                  ArenaMemoryResource& arena_)
        : parser_ir(std::move(parser_ir_)),
//          cmdman(&cmdman_),
          symrepo(&symrepo_),
          diag(&diag),
          arena(&arena_)
    {
    }

    void pass_declarations();
    void pass_analyze();

private:
    struct VarSubscript
    {
        std::string_view name;
        SourceRange source;
        std::optional<int32_t> literal;
    };

    struct VarRef
    {
        std::string_view name;
        SourceRange source;
        std::optional<VarSubscript> subscript;
    };

    void act_on_label_def(const ParserIR::LabelDef& label_def);

    void act_on_var_decl(const ParserIR::Command& command,
                         ScopeId scope_id,
                         SymVariable::Type type);

    /// This function parses a variable reference (encoded as an identifier).
    /// 
    /// In case of failure, it produces diagnostics and recovers. As such,
    /// this function never fails, but may produce outputs that do not
    /// coincide to what the user actually wrote in the source code.
    auto parse_var_ref(std::string_view identifier, SourceRange source)
        -> VarRef;

    auto report(SourceRange source, Diag message) -> DiagnosticBuilder;

private:
    ArenaMemoryResource* arena;
    DiagnosticHandler* diag;
    SymbolRepository* symrepo;
//    const CommandManager* cmdman;
    LinkedIR<ParserIR> parser_ir;
};
}
