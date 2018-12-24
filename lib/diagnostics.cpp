#include <gta3sc/diagnostics.hpp>
#include <cassert>

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
}
