// RUN: %gta3sc-filecheck %s --config=gta3 -emit-ir2 -o -
// RUN: %gta3sc-filecheck %s --config=gtavc -emit-ir2 -o -
// RUN: %not %gta3sc %s --config=gta3 -emit-ir2 -o - -pedantic-errors
// RUN: %not %gta3sc %s --config=gtavc -emit-ir2 -o - -pedantic-errors
{
VAR_INT gi
LVAR_INT li
VAR_FLOAT gf
LVAR_FLOAT lf

// CHECK: ANDOR 21i8
// CHECK-NEXT: IS_INT_VAR_EQUAL_TO_INT_VAR 0@ &8
// CHECK-NEXT: IS_FLOAT_VAR_EQUAL_TO_FLOAT_VAR 1@ &12
IF li = gi
OR lf = gf
    NOP
ENDIF

TERMINATE_THIS_SCRIPT
}