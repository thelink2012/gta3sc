// RUN: %gta3sc-verify %s --config=gta3 -fsyntax-only
// RUN: %gta3sc-verify %s --config=gtavc -fsyntax-only
PRINT_HELP abc
PRINT_HELP _abc // expected-error {{invalid identifier}}
PRINT_HELP semi: // expected-error {{invalid identifier}}
TERMINATE_THIS_SCRIPT