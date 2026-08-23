// Constants takes precedence over variables!
// RUN: %gta3sc %s --config=gtavc -fno-constant-checks --datadir=../semantics/Inputs/data -emit-ir2 -o - | %FileCheck %s

VAR_INT on
VAR_INT red
VAR_INT cheetah
VAR_INT lv_object

// CHECK-NEXT: #DEFINE_MODEL LV_OBJECT -1
// CHECK-NEXT: CREATE_OBJECT -1i8 0x0.000000p+0f 0x0.000000p+0f 0x0.000000p+0f &20
CREATE_OBJECT lv_object 0.0 0.0 0.0 lv_object

// CHECK-NEXT: CREATE_CAR 400i16 0x0.000000p+0f 0x0.000000p+0f 0x0.000000p+0f &16
CREATE_CAR cheetah 0.0 0.0 0.0 cheetah

// CHECK-NEXT: CREATE_PLAYER 0i8 0x0.000000p+0f 0x0.000000p+0f 0x0.000000p+0f 1i8
CREATE_PLAYER 0 0.0 0.0 0.0 on // global constant

// CHECK-NEXT: ADD_BLIP_FOR_CAR &16 &12
ADD_BLIP_FOR_CAR cheetah red

// CHECK-NEXT: CHANGE_BLIP_COLOUR &12 0i8
CHANGE_BLIP_COLOUR red red

// CHECK-NEXT: SET_PLAYER_CONTROL 1i8 1i8
SET_PLAYER_CONTROL on on // global constant

TERMINATE_THIS_SCRIPT