// RUN: %dis %gta3sc %s --config=gtasa --guesser --cs -fsyntax-only 2>&1 | %verify %s
SCRIPT_START
START_NEW_SCRIPT script_label // expected-error {{not allowed in custom scripts}}
SCRIPT_END

{
script_label:
TERMINATE_THIS_SCRIPT // expected-error {{not allowed in custom scripts}}
}
