// RUN: %gta3sc %s --config=gtavc -emit-ir2 -o - | %FileCheck %s

// TODO test all the expression alternators (maybe in alternators.sc)

VAR_INT   i j k
VAR_FLOAT x y z

// Test commutation property
{
    // CHECK:      MULT_INT_VAR_BY_VAL &8 2i8
    i = 2 * i
    // CHECK-NEXT: MULT_INT_VAR_BY_VAL &8 2i8
    i = i * 2
    // CHECK-NEXT: ADD_VAL_TO_INT_VAR &8 2i8
    i = 2 + i
    // CHECK-NEXT: ADD_VAL_TO_INT_VAR &8 2i8
    i = i + 2
}

//
{
    // CHECK-NEXT: SUB_INT_VAR_FROM_INT_VAR &8 &8
    i = i - i
    // CHECK-NEXT: SUB_VAL_FROM_INT_VAR &8 4i8
    i = i - 4
    // CHECK-NEXT: SET_VAR_INT_TO_VAR_INT &8 &12
    // CHECK-NEXT: SUB_VAL_FROM_INT_VAR &8 5i8
    i = j - 5
}

// Test VAR = CONST
{
    // CHECK: IS_INT_VAR_GREATER_THAN_CONSTANT &16 {{[\di]+}}
    // CHECK: IS_INT_VAR_EQUAL_TO_CONSTANT &16 {{[\di]+}}
    IF k > FBI
    OR k = ARMY
        // CHECK: SET_VAR_INT_TO_CONSTANT &16 {{[\di]+}}
        k = SWAT
    ENDIF
}

// Test Increment/Decrement
{
    // CHECK:      ADD_VAL_TO_INT_VAR &8 1i8
    ++i
    // CHECK-NEXT: SUB_VAL_FROM_INT_VAR &8 1i8
    --i
}

TERMINATE_THIS_SCRIPT