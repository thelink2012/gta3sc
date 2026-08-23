// Tests the -fswitch flag.
// RUN: %dis %gta3sc %s --config=gta3 -fsyntax-only 2>&1 | %verify %s
// RUN: %not %gta3sc %s --config=gta3 -fsyntax-only -fswitch 2>&1 | grep "guesser mode"
// RUN:      %gta3sc %s --config=gta3 -fsyntax-only -fswitch --guesser 2>&1
// RUN:      %gta3sc %s --config=gtasa -fsyntax-only --guesser 2>&1
VAR_INT n
SWITCH n // expected-error {{[-fswitch]}}
    CASE 1
        BREAK
ENDSWITCH
TERMINATE_THIS_SCRIPT