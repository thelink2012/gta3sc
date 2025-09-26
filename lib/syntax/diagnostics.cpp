#include <gta3sc/diagnostics.hpp>
#include <gta3sc/syntax/diagnostics.hpp>

namespace gta3sc::syntax::diag
{
const DiagnosticDescriptor expected_identifier(DiagnosticSeverity::error,
                                               "Expected identifier", "TODO");
const DiagnosticDescriptor expected_word(DiagnosticSeverity::error,
                                         "Expected word", "TODO");
const DiagnosticDescriptor too_many_arguments(DiagnosticSeverity::error,
                                              "Too many arguments", "TODO");
const DiagnosticDescriptor too_few_arguments(DiagnosticSeverity::error,
                                             "Too few arguments", "TODO");
const DiagnosticDescriptor integer_literal_too_big(DiagnosticSeverity::error,
                                                   "Integer literal too big",
                                                   "TODO");
} // namespace gta3sc::syntax::diag
