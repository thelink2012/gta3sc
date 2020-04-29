#include <cassert>
#include <gta3sc/diagnostics.hpp>

namespace gta3sc
{
DiagnosticBuilder::~DiagnosticBuilder() noexcept
{
    if(diag != nullptr)
        this->handler->emit(std::move(diag));
}

void DiagnosticHandler::emit(std::unique_ptr<Diagnostic> diag)
{
    assert(diag != nullptr);
    emitter(*diag);
}
} // namespace gta3sc
