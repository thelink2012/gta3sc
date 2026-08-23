// RUN: %gta3sc-verify %s --config=gta3 -fsyntax-only
VAR_INT x

IF x = 1 // expected-error {{use of more than 8 conditions}}
OR x = 2
OR x = 3
OR x = 4
OR x = 5
OR x = 6
OR x = 7
OR x = 8
OR x = 9
	WAIT 0
ENDIF

WHILE x = 1 // expected-error {{use of more than 8 conditions}}
OR x = 2
OR x = 3
OR x = 4
OR x = 5
OR x = 6
OR x = 7
OR x = 8
OR x = 9
	WAIT 0
ENDWHILE

IFNOT x = 1 // expected-error {{use of more than 8 conditions}}
OR x = 2
OR x = 3
OR x = 4
OR x = 5
OR x = 6
OR x = 7
OR x = 8
OR x = 9
	WAIT 0
ENDIF

WHILENOT x = 1 // expected-error {{use of more than 8 conditions}}
OR x = 2
OR x = 3
OR x = 4
OR x = 5
OR x = 6
OR x = 7
OR x = 8
OR x = 9
	WAIT 0
ENDWHILE

NOT x < 4 // expected-error {{NOT outside of a conditional statement}}

TERMINATE_THIS_SCRIPT