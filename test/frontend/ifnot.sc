// Tests the -fifnot flag.
// RUN: %dis %gta3sc %s --config=gtavc -fsyntax-only 2>&1 | %verify %s
// RUN:      %gta3sc %s --config=gtavc -fsyntax-only -fifnot 2>&1
// RUN:      %gta3sc %s --config=gta3 -fsyntax-only 2>&1
VAR_INT n
IFNOT n = 0 // expected-error {{[-fifnot]}}
    WAIT 0
ENDIF
WHILENOT n = 0 // expected-error {{[-fifnot]}}
    WAIT 0
ENDWHILE
TERMINATE_THIS_SCRIPT
