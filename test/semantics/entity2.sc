// RUN: %dis %gta3sc %s --config=gtavc -fsyntax-only 2>&1 | %verify %s

VAR_INT car

CREATE_CAR 0 .0 .0 .0 car

START_NEW_SCRIPT script1 car car
GOSUB bottom_of_script

TERMINATE_THIS_SCRIPT

{
	script1:

	LVAR_INT x y z

	CREATE_CAR 0 .0 .0 .0 x
	CREATE_CHAR 0 0 .0 .0 .0 y	// expected-error {{variable has already been used to create a entity of type CAR}}

	TERMINATE_THIS_SCRIPT
}

{
	script2:

	LVAR_INT x y z

	CREATE_CAR 0 .0 .0 .0 x
	CREATE_CHAR 0 0 .0 .0 .0 y

	TERMINATE_THIS_SCRIPT
}

bottom_of_script:
START_NEW_SCRIPT script2 car car // expected-error {{entity type mismatch in target label}}
RETURN
