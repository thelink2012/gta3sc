// RUN: %dis %gta3sc %s --config=gta3 -fsyntax-only 2>&1 | %verify %s

VAR_INT x

// SAVE_STRING_TO_DEBUG_FILE "no error expected

SAVE_STRING_TO_DEBUG_FILE "// expected-error {{terminating}}
/*
    SAVE_STRING_TO_DEBUG_FILE "no error expected
    /*
        SAVE_STRING_TO_DEBUG_FILE "no error expected
        /*
            SAVE_STRING_TO_DEBUG_FILE "no error expected
        */
        SAVE_STRING_TO_DEBUG_FILE "no error expected
    */
    SAVE_STRING_TO_DEBUG_FILE "no error expected
*/

/* 0 */ x = /* ?? */ 0 /* + ?? */
x = 0 // + ??

TERMINATE_THIS_SCRIPT

*/ // expected-error {{no comment to close}}

/*
// expected-error@comments.sc:0 {{unterminated /* comment}}
