// Regression Test 3
// Checks whether call scopes in SA Missions have their local vars starting at 0.
//
// RUN: %gta3sc %s --config=gtasa --guesser --cm -emit-ir2 -o - | %FileCheck %s
//
MISSION_START
{
LVAR_INT x
// CHECK: CLEO_CALL %MAIN_1 1i8 34@ 34@
CLEO_CALL proc1 0 x x
}
MISSION_END

{
proc1:
LVAR_INT x
// CHECK: CLEO_RETURN 1i8 0@
CLEO_RETURN 0 x
}
