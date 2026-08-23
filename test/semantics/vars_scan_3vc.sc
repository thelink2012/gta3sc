// RUN: %gta3sc-verify %s --config=gta3 -fsyntax-only
// RUN: %gta3sc-verify %s --config=gtavc -fsyntax-only

VAR_TEXT_LABEL var1 // expected-error {{text label variables are not supported}}
VAR_TEXT_LABEL var2 // expected-error {{text label variables are not supported}}

VAR_INT array[2] // expected-error {{arrays are not supported}}

TERMINATE_THIS_SCRIPT