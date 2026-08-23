// RUN: %gta3sc %s --config=gta3 -emit-ir2 -o - | %FileCheck %s
// RUN: %gta3sc %s --config=gtavc -emit-ir2 -o - | %FileCheck %s

{
    // CHECK: SET_LVAR_INT 16@ 1i8
    timera = 1
    // CHECK: SET_LVAR_INT 17@ 2i8
    timerb = 2
}

TERMINATE_THIS_SCRIPT