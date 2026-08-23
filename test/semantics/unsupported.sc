// RUN: %dis %gta3sc %s --config=gta3 -fsyntax-only 2>&1 | %verify %s

MAKE_PLAYER_SAFE 0 	// expected-error {{unsupported}}
IF WHILE 0 			// expected-error {{internal}}
OR IS_PLAYER_PLAYING 0
    WAIT 0
ENDIF

TERMINATE_THIS_SCRIPT