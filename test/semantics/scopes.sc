// RUN: %gta3sc-verify %s --config=gta3 -fsyntax-only

{
    LVAR_INT x
    {              // expected-error {{already inside a scope}}
        LVAR_INT x // expected-error {{exists}}
    }
}

LVAR_INT y // expected-error {{local variable definition outside of scope}}

TERMINATE_THIS_SCRIPT