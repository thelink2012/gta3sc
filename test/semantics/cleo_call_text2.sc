// RUN: %dis %gta3sc %s --config=gtasa --guesser --cs -fsyntax-only 2>&1 | %verify %s
SCRIPT_START
{
LVAR_TEXT_LABEL textvar

CLEO_CALL receives_text1 0 "text" // expected-error {{LVAR_TEXT_LABEL is not allowed}}

CLEO_CALL receives_text2 0 "text"    // expected-error {{unspecified}}
CLEO_CALL receives_text2 0 text      // expected-error {{unspecified}}
CLEO_CALL receives_text2 0 $textvar  // expected-error {{mismatch}}

TERMINATE_THIS_CUSTOM_SCRIPT
}

{
	receives_text1:
	LVAR_TEXT_LABEL textvar
	CLEO_RETURN 0
}

{
	receives_text2:
	LVAR_INT text_ptr
	CLEO_RETURN 0
}
SCRIPT_END