// RUN: %dis %gta3sc %s --config=gta3 -fsyntax-only 2>&1 | %verify %s

VAR_INT y

y = 4 -5    // expected-error {{expected newline after this token}}
y = 4 - 5
y = 4 - -5
y = 4 +5
y = 4 /5

TERMINATE_THIS_SCRIPT