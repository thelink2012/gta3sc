// RUN: %gta3sc %s --config=gta3 -emit-ir2 -o - | %FileCheck %s

{
    VAR_INT  a b c
    LVAR_INT x y z

    // CHECK-NEXT: SET_LVAR_INT_TO_LVAR_INT 0@ 2@
    x = z
    // CHECK-NEXT: SET_VAR_INT_TO_VAR_INT &8 &16
    a = c
}

{
    VAR_INT  d e f
    LVAR_INT i j k

    // CHECK-NEXT: SET_LVAR_INT_TO_LVAR_INT 0@ 2@
    i = k
    // CHECK-NEXT: SET_VAR_INT_TO_VAR_INT &20 &28
    d = f
}

TERMINATE_THIS_SCRIPT