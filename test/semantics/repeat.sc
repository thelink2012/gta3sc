// RUN: %gta3sc-verify %s --config=gta3 -fsyntax-only
{
VAR_INT x v
LVAR_INT y

REPEAT 5 x // fine
ENDREPEAT

REPEAT 5.0 x // expected-error {{expected integer}}
ENDREPEAT

REPEAT 5 y // fine by extension
ENDREPEAT



REPEAT v x // expected-error {{variable not allowed for this argument}}
ENDREPEAT
}
TERMINATE_THIS_SCRIPT