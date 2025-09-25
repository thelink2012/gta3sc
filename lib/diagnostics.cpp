#include <gta3sc/diagnostics.hpp>

namespace gta3sc::diag
{
const DiagnosticDescriptor internal_compiler_error(DiagnosticSeverity::error,
                                                   "Internal compiler error",
                                                   "TODO");
const DiagnosticDescriptor could_not_open_file(DiagnosticSeverity::error,
                                               "Could not open file", "TODO");
} // namespace gta3sc::diag

namespace gta3sc
{
Diagnostic::Builder::~Builder()
{
    if(this->target != nullptr)
    {
        this->target->emit(std::move(diag));
    }
}
} // namespace gta3sc
