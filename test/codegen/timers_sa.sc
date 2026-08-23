// RUN: %gta3sc-filecheck %s --config=gtasa --guesser -emit-ir2 -o -

{
    // CHECK: SET_LVAR_INT 32@ 1i8
    timera = 1
    // CHECK: SET_LVAR_INT 33@ 2i8
    timerb = 2
}

TERMINATE_THIS_SCRIPT