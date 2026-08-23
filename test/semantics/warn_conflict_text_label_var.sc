// RUN: %gta3sc-verify %s --config=gtasa --guesser -fsyntax-only

VAR_INT int
VAR_TEXT_LABEL text8

PRINT_HELP INT		// expected-warning {{text label collides with some variable name}}
PRINT_HELP TEXT8	// expected-warning {{text label collides with some variable name}}
PRINT_HELP $text8

TERMINATE_THIS_SCRIPT