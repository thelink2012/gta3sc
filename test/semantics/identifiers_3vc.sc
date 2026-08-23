// RUN: %dis %gta3sc %s --config=gta3 -fsyntax-only 2>&1 | %verify %s
// RUN: %dis %gta3sc %s --config=gtavc -fsyntax-only 2>&1 | %verify %s
PRINT_HELP abc
PRINT_HELP _abc // expected-error {{invalid identifier}}
PRINT_HELP semi: // expected-error {{invalid identifier}}
TERMINATE_THIS_SCRIPT