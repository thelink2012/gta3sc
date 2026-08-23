// RUN: %dis %gta3sc %s --config=gta3 -fsyntax-only 2>&1 | %verify %s

{
    LVAR_INT x
    {              // expected-error {{already inside a scope}}
        LVAR_INT x // expected-error {{exists}}
    }
}

LVAR_INT y // expected-error {{local variable definition outside of scope}}

TERMINATE_THIS_SCRIPT