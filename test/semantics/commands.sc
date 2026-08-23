// Commands Matching Errors
// RUN: %dis %gta3sc %s --config=gtasa --guesser -fsyntax-only 2>&1 | %verify %s
{
VAR_INT car
VAR_INT i j k a[10] b[10]
VAR_FLOAT x y z
LVAR_INT lvar

QWERTY 1 2 3 			// expected-error {{unknown command}}
SET 1 i 				// expected-error {{could not match alternative}}

CREATE_CAR 0 .0 .0 .0 car car // expected-error {{too many arguments}}
CREATE_CAR 0 .0 .0 .0         // expected-error {{too few arguments}}

WAIT 0.0 					// expected-error {{expected integer}}
SET_TIME_SCALE 0 			// expected-error {{expected float}}
SAVE_STRING_TO_DEBUG_FILE q // expected-error {{expected string literal}}

GOTO 0              // expected-error {{no label with this name}}
GOTO 0.0            // expected-error {{no label with this name}}
GOTO dummy 			// expected-error {{no label with this name}}

PRINT_HELP 0        // expected-error {{expected text label}}
PRINT_HELP 0.0      // expected-error {{expected text label}}
PRINT_HELP $dummy 	// expected-error {{expected variable}}

IS_INT_VAR_EQUAL_TO_CONSTANT i DUMMY // expected-error {{no string constant with this name}}
IS_INT_VAR_EQUAL_TO_CONSTANT i 0 // fine
IS_INT_VAR_EQUAL_TO_CONSTANT i 0.0 // expected-error {{no string constant with this name}}

START_NEW_SCRIPT script_label 0 0.0 DUMMY // expected-error {{no variable with this name}}

WAIT dummy 			// expected-error {{no variable with this name}}
WAIT !dummy         // expected-error {{invalid identifier}}

WAIT x              // expected-error {{variable type does not match argument type}}
SET_VAR_INT lvar 0  // expected-error {{variable kind (global/local) not allowed for this argument}}
SET_VAR_INT i j     // expected-error {{variable not allowed for this argument}}

WAIT a       // expected-error {{use of array variable without a index}}
WAIT a[b]    // expected-error {{variable in index is of array type}}
WAIT a[dummy]// expected-error {{identifier between brackets is not a variable}}
WAIT a[b[0]] // expected-error {{nesting of arrays not allowed}}
WAIT a[-1]   // expected-error {{index cannot be negative}}
WAIT a[10]   // expected-error {{index out of range}}
WAIT a[x]    // expected-error {{variable in index is not of INT type}}
WAIT a[i]    // fine
WAIT a[MISSION_NONE] // extension (gives error on -pedantic)
WAIT i[3]     // expected-error {{variable is not array}}

PRINT_HELP "bad"		// expected-error {{string literal not allowed here}}
REQUEST_ANIMATION "bad" // expected-error {{string literals on arguments are disallowed [-fcleo]}}

CONST_INT my_int 0
CONST_FLOAT my_flt 0.0

WAIT my_flt           // expected-error {{user constant is not of integer type}}
SET_TIME_SCALE my_int // expected-error {{user constant is not of float type}}
IS_INT_VAR_EQUAL_TO_CONSTANT i my_int // expected-error {{no string constant with this name}}

}
TERMINATE_THIS_SCRIPT

{
script_label:
LVAR_INT s1
LVAR_FLOAT s2
LVAR_INT s3
TERMINATE_THIS_SCRIPT
}