// RUN: %dis %gta3sc %s --config=gta3 -fswitch --guesser -emit-ir2 -o - 2>&1 | %verify %s
// RUN: %dis %gta3sc %s --config=gtavc -fswitch --guesser -emit-ir2 -o - 2>&1 | %verify %s
VAR_INT n

SWITCH n // expected-error {{more than 8 cases}}
	CASE 1
	CASE 2
	CASE 3
	CASE 4
	CASE 5
	CASE 6
	CASE 7
	CASE 8
	CASE 9
		BREAK
ENDSWITCH

TERMINATE_THIS_SCRIPT