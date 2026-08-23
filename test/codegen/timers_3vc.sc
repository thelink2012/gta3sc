// RUN: %gta3sc-filecheck %s --config=gta3 -emit-ir2 -o -
// RUN: %gta3sc-filecheck %s --config=gtavc -emit-ir2 -o -

{
    // CHECK: SET_LVAR_INT 16@ 1i8
    timera = 1
    // CHECK: SET_LVAR_INT 17@ 2i8
    timerb = 2
}

TERMINATE_THIS_SCRIPT