// RUN: %dis %gta3sc %s --config=gtasa --guesser -pedantic -fsyntax-only 2>&1 | %verify %s
VAR_INT n j
VAR_FLOAT f

SWITCH n
    CASE 1
    CASE 1      // expected-error {{duplicate CASE}}
        BREAK
    CASE 2      // expected-error {{does not end with a BREAK}}
        WAIT 0  
    CASE 3
        BREAK
    CASE j      // expected-error {{variable not allowed}}
        BREAK
    CASE 1.0    // expected-error {{expected integer}}
        BREAK
    DEFAULT
        BREAK
    DEFAULT    // expected-error {{multiple DEFAULT labels in one SWITCH}}
ENDSWITCH

SWITCH f // expected-error {{variable type does not match argument type}}
ENDSWITCH

SWITCH n
    BREAK // expected-error {{BREAK not within a CASE or DEFAULT label}}
ENDSWITCH

BREAK // expected-error {{BREAK only allowed at the end of a SWITCH CASE}}

SWITCH n // expected-error {{SWITCH contains more than 75 cases}}
	CASE 0
	CASE 1
	CASE 2
	CASE 3
	CASE 4
	CASE 5
	CASE 6
	CASE 7
	CASE 8
	CASE 9
	CASE 10
	CASE 11
	CASE 12
	CASE 13
	CASE 14
	CASE 15
	CASE 16
	CASE 17
	CASE 18
	CASE 19
	CASE 20
	CASE 21
	CASE 22
	CASE 23
	CASE 24
	CASE 25
	CASE 26
	CASE 27
	CASE 28
	CASE 29
	CASE 30
	CASE 31
	CASE 32
	CASE 33
	CASE 34
	CASE 35
	CASE 36
	CASE 37
	CASE 38
	CASE 39
	CASE 40
	CASE 41
	CASE 42
	CASE 43
	CASE 44
	CASE 45
	CASE 46
	CASE 47
	CASE 48
	CASE 49
	CASE 50
	CASE 51
	CASE 52
	CASE 53
	CASE 54
	CASE 55
	CASE 56
	CASE 57
	CASE 58
	CASE 59
	CASE 60
	CASE 61
	CASE 62
	CASE 63
	CASE 64
	CASE 65
	CASE 66
	CASE 67
	CASE 68
	CASE 69
	CASE 70
	CASE 71
	CASE 72
	CASE 73
	CASE 74
    CASE 75
		BREAK
ENDSWITCH

TERMINATE_THIS_SCRIPT