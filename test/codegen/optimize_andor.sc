// RUN: %gta3sc-filecheck %s --config=gta3 -moptimize-andor -emit-ir2 -o -

VAR_INT x

// CHECK-NOT: ANDOR 0i8
// CHECK:     IS_INT_VAR_EQUAL_TO_NUMBER &8 0i8
IF x = 0
    WAIT 0
ENDIF

// CHECK-NOT: ANDOR 0i8
// CHECK:     IS_INT_VAR_EQUAL_TO_NUMBER &8 0i8
WHILE x = 0
    WAIT 0
ENDWHILE

TERMINATE_THIS_SCRIPT

