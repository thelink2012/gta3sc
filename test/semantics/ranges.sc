// RUN: %dis %gta3sc %s --config=gta3 -fsyntax-only 2>&1 | %verify %s

WAIT 0

WAIT -2147483648
WAIT -2147483649 // expected-error {{out of range}}

WAIT 2147483647
WAIT 2147483648 // expected-error {{out of range}}

WAIT 0x1
WAIT -0x1 // expected-error {{invalid identifier}}

WAIT 0x7FFFFFFF
WAIT 0x80000000

WAIT 0xFFFFFFFF
WAIT 0x100000000 // expected-error {{out of range}}


TERMINATE_THIS_SCRIPT
