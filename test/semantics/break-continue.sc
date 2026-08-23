// RUN: %dis %gta3sc %s --config=gtasa --guesser -fbreak-continue -fsyntax-only 2>&1 | %verify %s
VAR_INT x

CONTINUE  // expected-error {{CONTINUE not in a loop}}
BREAK     // expected-error {{BREAK not in a loop or SWITCH statement}}

IF x = 0
	CONTINUE // expected-error {{CONTINUE not in a loop}}
	BREAK    // expected-error {{BREAK not in a loop}}
ELSE
	CONTINUE // expected-error {{CONTINUE not in a loop}}
	BREAK    // expected-error {{BREAK not in a loop}}
ENDIF

WHILE x = 0
	CONTINUE
	BREAK
ENDWHILE

REPEAT 5 x
	CONTINUE
	BREAK
ENDREPEAT

SWITCH x
	CASE 1
		CONTINUE // expected-error {{CONTINUE not in a loop}}
		BREAK
ENDSWITCH

TERMINATE_THIS_SCRIPT