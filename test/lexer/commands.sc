// RUN: %dis %gta3sc %s --config=gta3 -fsyntax-only 2>&1 | %verify %s

IF {        // expected-error {{unexpected token}}
OR ELSE     // expected-error {{unexpected token}}
OR LVAR_INT // expected-error {{unexpected token}}
ENDIF

TERMINATE_THIS_SCRIPT