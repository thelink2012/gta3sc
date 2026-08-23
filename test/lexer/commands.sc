// RUN: %gta3sc-verify %s --config=gta3 -fsyntax-only

IF {        // expected-error {{unexpected token}}
OR ELSE     // expected-error {{unexpected token}}
OR LVAR_INT // expected-error {{unexpected token}}
ENDIF

TERMINATE_THIS_SCRIPT