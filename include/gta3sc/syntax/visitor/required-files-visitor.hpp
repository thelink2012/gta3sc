#pragma once
#include <functional>
#include <gta3sc/ir/instruction-visitor.hpp>
#include <gta3sc/ir/parser-ir.hpp>
#include <gta3sc/ir/symbol-table.hpp>

namespace gta3sc::syntax
{
/// Visitor that analyzes IR to find required files.
///
/// This visitor traverses parser IR instructions and identifies commands that
/// require external files (GOSUB_FILE, LAUNCH_MISSION,
/// LOAD_AND_LAUNCH_MISSION). When such commands are found, the visitor calls
/// the virtual \ref visit_require method with the command, argument, and file
/// type of the required file.
///
/// See also \ref CallbackRequiredFilesVisitor if you want to pass a callback
/// instead of implementing a virtual method.
class RequiredFilesVisitor : public InstructionVisitor<ParserIR>
{
public:
    RequiredFilesVisitor() noexcept = default;
    ~RequiredFilesVisitor() noexcept override = default;

    RequiredFilesVisitor(const RequiredFilesVisitor&) noexcept = delete;
    auto operator=(const RequiredFilesVisitor&) noexcept
            -> RequiredFilesVisitor& = delete;

    RequiredFilesVisitor(RequiredFilesVisitor&&) noexcept = default;
    auto operator=(RequiredFilesVisitor&&) noexcept
            -> RequiredFilesVisitor& = default;

    /// Visits an instruction and analyzes it for required files.
    void visit(const ParserIR&) override;

protected:
    /// Called when a file requirement is found.
    ///
    /// \param command the command that requires the file.
    /// \param arg the argument containing the filename.
    ///    Guaranteed to be a filename argument (i.e. as_filename() is not
    ///    null).
    /// \param file_type the type of the required file.
    virtual void visit_require(const ParserIR::Command& command,
                               const ParserIR::Argument& arg,
                               SymbolTable::FileType file_type)
            = 0;

private:
    void visit_require(const ParserIR::Command& command, uint32_t arg_index,
                       SymbolTable::FileType file_type);
};

/// Callback-based implementation of \ref RequiredFilesVisitor.
///
/// This visitor uses a std::function callback to inform about required files
/// rather than requiring a derived class.
class CallbackRequiredFilesVisitor final : public RequiredFilesVisitor
{
public:
    using CallbackFunction = std::function<void(const ParserIR::Command&,
                                                const ParserIR::Argument&,
                                                SymbolTable::FileType)>;

    explicit CallbackRequiredFilesVisitor(CallbackFunction callback) noexcept;

    CallbackRequiredFilesVisitor(const CallbackRequiredFilesVisitor&) = delete;
    auto operator=(const CallbackRequiredFilesVisitor&)
            -> CallbackRequiredFilesVisitor& = delete;

    CallbackRequiredFilesVisitor(CallbackRequiredFilesVisitor&&) noexcept
            = default;
    auto operator=(CallbackRequiredFilesVisitor&&) noexcept
            -> CallbackRequiredFilesVisitor& = default;

    ~CallbackRequiredFilesVisitor() noexcept override = default;

protected:
    void visit_require(const ParserIR::Command& command,
                       const ParserIR::Argument& arg,
                       SymbolTable::FileType file_type) override;

private:
    CallbackFunction callback;
};
} // namespace gta3sc::syntax
