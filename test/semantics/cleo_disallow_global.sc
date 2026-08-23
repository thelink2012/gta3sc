// RUN: %gta3sc-verify %s --config=gtasa --guesser --cs -fsyntax-only
SCRIPT_START
{
VAR_INT x // expected-error {{declaring global variables in custom scripts}}
TERMINATE_THIS_CUSTOM_SCRIPT
}
SCRIPT_END