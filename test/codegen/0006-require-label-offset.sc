// Regression Test 6
// Checks whether local offsets of required scripts (second require in special) are correct.
//
// RUN: %gta3sc %s --config=gtasa --guesser --cs -emit-ir2 -o - | %FileCheck %s

SCRIPT_START
REQUIRE a.sc
REQUIRE b.sc
// CHECK: GOSUB %MAIN_1
GOSUB label_here
// CHECK: GOSUB %MAIN_2
GOSUB alabel
// CHECK: GOSUB %MAIN_3
GOSUB blabel
SCRIPT_END

// CHECK: MAIN_1:
label_here:
// CHECK: RETURN
RETURN

// from a.sc
// CHECK: MAIN_2:
// CHECK: RETURN

// from b.sc
// CHECK: MAIN_3:
// CHECK: RETURN
