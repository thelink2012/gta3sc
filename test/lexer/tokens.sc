// RUN: %dis %gta3sc %s --config=gtasa --guesser -fsyntax-only 2>&1 | %verify %s

VAR_TEXT_LABEL ab3

// Integer Tokens
WAIT 1
WAIT -1
WAIT 5-2 // valid, same as 5

// Float Tokens
SET_TIME_SCALE .0
SET_TIME_SCALE 1.0
SET_TIME_SCALE 1.0f
SET_TIME_SCALE 1.0F
SET_TIME_SCALE -1.0
SET_TIME_SCALE 1.0.9 // valid, same as 1.0
SET_TIME_SCALE 1.0f9 // valid, same as 1.0
SET_TIME_SCALE 1.0-9 // e//xpected-error {{invalid identifier}}
                     // the token above is not a float!
                     // also commented the error expect because that check is on semantic analyzes

// Text Tokens
PRINT_HELP ab3
PRINT_HELP ab3^?
PRINT_HELP WHILE

// String Tokens
SAVE_STRING_TO_DEBUG_FILE "A string to rule them all"
SAVE_STRING_TO_DEBUG_FILE "abc // expected-error-re {{missing terminating '.' character}}

TERMINATE_THIS_SCRIPT