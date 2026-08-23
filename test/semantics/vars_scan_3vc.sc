// RUN: %dis %gta3sc %s --config=gta3 -fsyntax-only 2>&1 | %verify %s
// RUN: %dis %gta3sc %s --config=gtavc -fsyntax-only 2>&1 | %verify %s

VAR_TEXT_LABEL var1 // expected-error {{text label variables are not supported}}
VAR_TEXT_LABEL var2 // expected-error {{text label variables are not supported}}

VAR_INT array[2] // expected-error {{arrays are not supported}}

TERMINATE_THIS_SCRIPT