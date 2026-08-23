// RUN: %dis %gta3sc %s --config=gtasa --guesser -fsyntax-only 2>&1 | %verify %s

VAR_INT a[10] b[10] i

WAIT a[1]
WAIT a[1]works   // same as WAIT a[1]
WAIT a[b[1]]     // expected-error {{nesting of arrays}}
WAIT a[b]        // expected-error {{index is of array type}}
WAIT a[i]

TERMINATE_THIS_SCRIPT