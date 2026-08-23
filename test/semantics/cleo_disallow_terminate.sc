// RUN: %gta3sc-verify %s --config=gtasa --guesser --cs -fsyntax-only
SCRIPT_START
START_NEW_SCRIPT script_label // expected-error {{not allowed in custom scripts}}
SCRIPT_END

{
script_label:
TERMINATE_THIS_SCRIPT // expected-error {{not allowed in custom scripts}}
}
