// RUN: %dis %gta3sc %s --config=gtasa --guesser --datadir=Inputs/data -fsyntax-only 2>&1 | %verify %s

					// expected-error@gosub1.sc:7 {{variable name exists already}}
VAR_INT dup_var		// expected-note {{previously defined here}}

					// expected-error@gosub1.sc:8 {{label name exists already}}
dup_label:			// expected-note {{previously defined here}}

VAR_INT dup_var_here
VAR_INT dup_var_here // expected-error {{variable name exists already}}
					 // expected-note@-2 {{previously defined here}}

dup_label_here:
dup_label_here: 	// expected-error {{label name exists already}}
					// expected-note@-2 {{previously defined here}}

VAR_INT dup_var_label
dup_var_label:		// fine

{
    LVAR_INT dup_local
}

{
    LVAR_INT dup_local
}

{
    LVAR_INT dup_global_local // expected-error {{variable name exists already}}
}                             // expected-note@gosub1.sc:10 {{previously defined here}}

VAR_INT timera        // expected-error {{variable name exists already}}
{
   LVAR_INT timerb    // expected-error {{variable name exists already}}
}

{
VAR_INT on         // expected-error {{variable name exists already as a string constant}}
LVAR_INT off       // expected-error {{variable name exists already as a string constant}}
VAR_INT red        // expected-error {{variable name exists already as a string constant}}
LVAR_INT green     // expected-error {{variable name exists already as a string constant}}
VAR_INT cheetah    // expected-error {{variable name exists already as a string constant}}
LVAR_INT infernus  // expected-error {{variable name exists already as a string constant}}
VAR_INT lv_building// expected-error {{variable name exists already as a string constant}}
LVAR_INT lv_object // expected-error {{variable name exists already as a string constant}}
VAR_INT gosub1_int // expected-error {{variable name exists already as a string constant}}
LVAR_INT gosub1_flt // expected-error {{variable name exists already as a string constant}}
}

{
CONST_INT DUP_INT_HERE 0
CONST_INT DUP_INT_HERE 1 // expected-error {{user constant exists already}}
                         // expected-note@-2: {{previously defined here}}

CONST_INT DUP_INT 0 	// expected-error@gosub1.sc:15 {{user constant exists already}}
						// expected-note@-1: {{previously defined here}}

CONST_FLOAT DUP_FLT 0.0 // expected-error@gosub1.sc:16 {{user constant exists already}}
						// expected-note@-1: {{previously defined here}}

CONST_INT PAD1 0		// expected-error {{user constant exists already as a string constant}}
CONST_INT OFF 0			// expected-error {{user constant exists already as a string constant}}
CONST_INT LV_STUFF 0	// expected-error {{user constant exists already as a string constant}}
CONST_FLOAT BANSHEE 0.0	// expected-error {{user constant exists already as a string constant}}
}

LAUNCH_MISSION gosub1.sc
TERMINATE_THIS_SCRIPT