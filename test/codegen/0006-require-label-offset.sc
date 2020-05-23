// Regression Test 6
// Checks whether local offsets of required scripts (second require in special) are correct.
//
// RUN: %gta3sc %s --config=gtasa --guesser --cs -emit-ir2 -o - | %FileCheck %s

SCRIPT_START
REQUIRE a.sc
REQUIRE b.sc
// CHECK-L: GOSUB %MAIN_1
GOSUB label_here
// CHECK-L: GOSUB %MAIN_2
GOSUB alabel
// CHECK-L: GOSUB %MAIN_3
GOSUB blabel
SCRIPT_END

// CHECK--L: MAIN_1:
label_here:
// CHECK-L: RETURN
RETURN

// from a.sc
// CHECK-L: MAIN_2:
// CHECK-L: RETURN

// from b.sc
// CHECK-L: MAIN_3:
// CHECK-L: RETURN
