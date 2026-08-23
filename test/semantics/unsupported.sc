// RUN: %gta3sc-verify %s --config=gta3 -fsyntax-only

MAKE_PLAYER_SAFE 0 	// expected-error {{unsupported}}
IF WHILE 0 			// expected-error {{internal}}
OR IS_PLAYER_PLAYING 0
    WAIT 0
ENDIF

TERMINATE_THIS_SCRIPT