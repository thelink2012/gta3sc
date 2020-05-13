#pragma once
#include <gta3sc/codegen/relocation-table.hpp>
#include <gta3sc/codegen/trilogy/emitter.hpp>
#include <gta3sc/diagnostics.hpp>
#include <gta3sc/ir/linked-ir.hpp>
#include <gta3sc/ir/sema-ir.hpp>

namespace gta3sc::codegen
{
class RelocationTable;
class StorageTable;
} // namespace gta3sc::codegen

namespace gta3sc::codegen::trilogy
{
/// Generates bytecode for GTA III, Vice City and San Andreas.
///
/// This class receives as input semantically valid and annotated IR and spits
/// bytecode and relocation information for the bytecode.
///
/// An instance of this can only generate bytecode for a *single* file. It
/// assumes the bytecode is at a certain input offset in the multi-file.
///
/// The bytecode is output into an `OutputIterator` and any necessary relocation
/// information (such as label offsets and label references) are registered
/// in a `RelocationTable`. This table must later on be scanned to perform
/// relocation of label references through the bytecode.
///
/// This code generator is headerless, please use `MultifileCodeGen` if
/// the generation of a header is necessary.
///
/// For emitting bytecode having no concern to language semantics, use
/// `CodeEmitter`.
class CodeGen
{
public:
    using AbsoluteOffset = typename RelocationTable::AbsoluteOffset;
    using RelativeOffset = typename RelocationTable::RelativeOffset;

public:
    /// Constructs a code generator for `file`.
    ///
    /// The code generation will assume its code is positioned at
    /// `multifile_offset` in the multi-file and use `storage` to obtain
    /// the index of variables used in the input IR.
    ///
    /// Any diagnostic produced during code generation will be handed to `diag`.
    CodeGen(const SymbolTable::File& file, AbsoluteOffset multifile_offset,
            const StorageTable& storage, DiagnosticHandler& diag) noexcept :
        diag(&diag),
        storage(&storage),
        file(&file),
        base_offset(multifile_offset)
    {}

    CodeGen(const CodeGen&) = delete;
    auto operator=(const CodeGen&) -> CodeGen& = delete;

    CodeGen(CodeGen&&) noexcept = default;
    auto operator=(CodeGen&&) noexcept -> CodeGen& = default;

    ~CodeGen() noexcept = default;

    /// Generates code for an instruction.
    ///
    /// Bytecode for `ir` will be generated in `output_iter` and any necessary
    /// relocation information will be registered in `reloc_table`.
    ///
    /// Returns an iterator past the last byte written in `output_iter` or
    /// `std::nullptr` in case of an error.
    template<typename OutputIterator>
    auto generate(const SemaIR& ir, RelocationTable& reloc_table,
                  OutputIterator output_iter) -> std::optional<OutputIterator>;

    /// Generates code for a list of instructions.
    ///
    /// Bytecode for `ir` will be generated in `output_iter` and any necessary
    /// relocation information will be registered in `reloc_table`.
    ///
    /// Returns an iterator past the last byte written in `output_iter` or
    /// `std::nullptr` in case of an error.
    template<typename OutputIterator>
    auto generate(const LinkedIR<SemaIR>& ir, RelocationTable& reloc_table,
                  OutputIterator output_iter) -> std::optional<OutputIterator>;

private:
    // The methods below generate code for the element given as first parameter
    // using `emitter` and returns an boolean on whether the generation was
    // successful. Any error is handed to `diag`. In case generation never
    // fails, it returns void.
    //
    // Passing a `RelocationTable` as parameter indicates the method may produce
    // relocation information.

    auto generate(const SemaIR& ir, RelocationTable& reloc_table) -> bool;

    auto generate_command(const SemaIR::Command& command,
                          RelocationTable& reloc_table) -> bool;

    void generate_argument(const SemaIR::Argument& arg,
                           const CommandTable::ParamDef& param,
                           RelocationTable& reloc_table);

    void generate_int(const SemaIR::Argument& arg);

    void generate_float(const SemaIR::Argument& arg);

    void generate_used_object(const SemaIR::Argument& arg);

    void generate_text_label(const SemaIR::Argument& arg,
                             const CommandTable::ParamDef& param);

    void generate_string(const SemaIR::Argument& arg,
                         const CommandTable::ParamDef& param);

    void generate_var_ref(const SemaIR::Argument& arg);

    void generate_label(const SemaIR::Argument& arg,
                        RelocationTable& reloc_table);

    void generate_filename_label(const SemaIR::Argument& arg,
                                 RelocationTable& reloc_table);

private:
    DiagnosticHandler* diag;
    const StorageTable* storage;
    const SymbolTable::File* file;
    AbsoluteOffset base_offset;
    CodeEmitter emitter;
};

template<typename OutputIterator>
inline auto CodeGen::generate(const SemaIR& ir, RelocationTable& reloc_table,
                              OutputIterator output_iter)
        -> std::optional<OutputIterator>
{
    this->emitter.buffer_clear();

    if(!generate(ir, reloc_table))
        return std::nullopt;

    return emitter.drain(output_iter);
}

template<typename OutputIterator>
inline auto CodeGen::generate(const LinkedIR<SemaIR>& linked_ir,
                              RelocationTable& reloc_table,
                              OutputIterator output_iter)
        -> std::optional<OutputIterator>
{
    bool has_any_error = false;
    for(const auto& ir : linked_ir)
    {
        if(auto result = generate(ir, reloc_table, output_iter); result)
            output_iter = *result;
        else
            has_any_error = true;
    }
    return !has_any_error ? std::optional(output_iter) : std::nullopt;
}
} // namespace gta3sc::codegen::trilogy
