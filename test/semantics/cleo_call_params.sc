// RUN: %dis %gta3sc %s --config=gtasa --guesser --cs -fsyntax-only 2>&1 | %verify %s
SCRIPT_START
{
LVAR_INT i j k
LVAR_FLOAT x y z

// Argument Count
CLEO_CALL sum_int 0 5 i j 	// this defines what the function is like
CLEO_CALL sum_int 0 5 i 0 	// expected-error {{expected a variable as output}}
CLEO_CALL sum_int 0 5 i   	// expected-error {{too few arguments for this function call}}
CLEO_CALL sum_int 0 5 i j k // expected-error {{too many arguments for this function call}}
CLEO_CALL sum_int 0 5 		// expected-error {{too few arguments for this function call}}
CLEO_CALL sum_int 0 		// expected-error {{too few arguments for this function call}}
CLEO_CALL sum_int 1 		// expected-error {{this argument shall be set to 0}}
							// expected-error@-1 {{too few arguments for this function call}}

// Argument Type
CLEO_CALL sum_int 0 5.0 i j 	// expected-error {{type mismatch in target label}}
CLEO_CALL sum_int 0 x i j 		// expected-error {{type mismatch in target label}}
CLEO_CALL sum_int 0 i j z 		// expected-error {{type mismatch}}

TERMINATE_THIS_CUSTOM_SCRIPT
}

{
	sum_int:
	LVAR_INT a b c
	LVAR_FLOAT x y z
	c = a + b
	CLEO_RETURN 0 c		// this defines what the return is like
	CLEO_RETURN 0		// expected-error {{too few return arguments}}
	CLEO_RETURN 0 c c	// expected-error {{too many return arguments}}
	CLEO_RETURN 1 c		// expected-error {{this argument shall be set to 0}}
	CLEO_RETURN 0 x		// expected-error {{type mismatch}}
}
SCRIPT_END