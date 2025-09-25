#pragma once

namespace gta3sc
{
class DiagnosticDescriptor;
} // namespace gta3sc

// Shared diagnostics between syntax modules.
namespace gta3sc::syntax::diag
{
extern const DiagnosticDescriptor expected_identifier;
extern const DiagnosticDescriptor expected_word; // %0 => string
extern const DiagnosticDescriptor
        too_many_arguments; // %0 => int (expected), %1 => int (got)
extern const DiagnosticDescriptor
        too_few_arguments; // %0 => int (expected), %1 => int (got)
extern const DiagnosticDescriptor integer_literal_too_big;
} // namespace gta3sc::syntax::diag
