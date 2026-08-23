// RUN: %gta3sc-filecheck %s --config=gta3 -emit-ir2 -o -
// RUN: %gta3sc-filecheck %s --config=gtavc -emit-ir2 -o -
// RUN: %gta3sc-filecheck %s --config=gtasa --guesser -emit-ir2 -o -
// RUN: %not %gta3sc %s --config=gta3 -emit-ir2 -o - -pedantic-errors
// RUN: %not %gta3sc %s --config=gtavc -emit-ir2 -o - -pedantic-errors
// RUN: %not %gta3sc %s --config=gtasa --guesser -emit-ir2 -o - -pedantic-errors
{
LVAR_INT lvar

// CHECK: ADD_VAL_TO_INT_LVAR 0@ 1i8
// CHECK: IS_INT_LVAR_GREATER_OR_EQUAL_TO_NUMBER 0@ 5i8

REPEAT 5 lvar
    NOP
ENDREPEAT

TERMINATE_THIS_SCRIPT
}