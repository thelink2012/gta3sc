// RUN: %dis %gta3sc %s --config=gtasa --guesser --cs -fsyntax-only 2>&1 | %verify %s
SCRIPT_START
{
LVAR_INT car char car2 null1 null2 null3 null4 

CREATE_CAR 0 .0 .0 .0 car
CREATE_CHAR 0 0 .0 .0 .0 char 

CLEO_CALL blow_up1 0 car null1
CLEO_CALL blow_up1 0 char null2 // expected-error {{entity type mismatch in target label}}

CLEO_CALL blow_up2 0 char null3
CLEO_CALL blow_up2 0 car null4 // expected-error {{entity type mismatch in target label}}

SET_CAR_HEALTH null1 1000	// fine
SET_CAR_HEALTH null2 1000	// fine
SET_CAR_HEALTH null3 1000	// expected-error {{expected variable of type CAR but got CHAR}}
SET_CAR_HEALTH null4 1000	// expected-error {{expected variable of type CAR but got CHAR}}

TERMINATE_THIS_CUSTOM_SCRIPT
}

{
	blow_up1:
	LVAR_INT car
	EXPLODE_CAR car
	CLEO_RETURN 0 car

	LVAR_INT char
	CREATE_CHAR 0 0 .0 .0 .0 char
	CLEO_RETURN 0 char // expected-error {{entity type mismatch}}
}

{
	blow_up2:
	LVAR_INT car
	EXPLODE_CAR car // expected-error {{expected variable of type CAR but got CHAR}}
	CLEO_RETURN 0 car

	LVAR_INT char
	CREATE_CHAR 0 0 .0 .0 .0 char
	CLEO_RETURN 0 char // fine
}
SCRIPT_END