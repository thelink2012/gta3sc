// RUN: %dis %gta3sc %s --config=gtasa --guesser --cs -fsyntax-only 2>&1 | %verify %s
SCRIPT_START
{
CLEO_CALL returns_text1 0
CLEO_CALL returns_text2 0
CLEO_CALL returns_text3 0
TERMINATE_THIS_CUSTOM_SCRIPT
}

{
	returns_text1:
	CLEO_RETURN 0 "text" // expected-error {{this output type is not supported}}
}

{
	returns_text2:
	CLEO_RETURN 0 text // expected-error {{this output type is not supported}}
}

{
	returns_text3:
	LVAR_TEXT_LABEL textvar
	CLEO_RETURN 0 $textvar // expected-error {{this output type is not supported}}
}
SCRIPT_END