// RUN: %gta3sc-filecheck %s --config=gtasa --guesser -emit-ir2 -o -
VAR_INT n

// CHECK: NOP
NOP

// Using a default case, and an out of order case.
{
    // CHECK: SWITCH_START &8 4i8 1i8 @MAIN_5 50i8 @MAIN_4 100i8 @MAIN_1 200i16 @MAIN_2 300i16 @MAIN_3 -1i8 @MAIN_6 -1i8 @MAIN_6 -1i8 @MAIN_6
    SWITCH n
        // CHECK-NEXT: MAIN_1:
        CASE 100
            // CHECK-NEXT: WAIT 100i8
            // CHECK-NEXT: GOTO @MAIN_6
            WAIT 100
            BREAK
        // CHECK-NEXT: MAIN_2:
        CASE 200
            // CHECK-NEXT: WAIT 200i16
            // CHECK-NEXT: GOTO @MAIN_6
            WAIT 200
            BREAK
        // CHECK-NEXT: MAIN_3:
        CASE 300
            // CHECK-NEXT: WAIT 300i16
            // CHECK-NEXT: GOTO @MAIN_6
            WAIT 300
            BREAK
        // CHECK-NEXT: MAIN_4:
        CASE 50
            // CHECK-NEXT: WAIT 50i8
            // CHECK-NEXT: GOTO @MAIN_6
            WAIT 50
            BREAK
        // CHECK-NEXT: MAIN_5:
        DEFAULT
            // CHECK-NEXT: WAIT 0i8
            // CHECK-NEXT: GOTO @MAIN_6
            WAIT 0
            BREAK
    ENDSWITCH
}


// Using no default case, and an out of order case.
{
    // CHECK-NEXT: MAIN_6:
    // CHECK-NEXT: SWITCH_START &8 3i8 0i8 @MAIN_10 50i8 @MAIN_9 100i8 @MAIN_7 200i16 @MAIN_8 -1i8 @MAIN_10 -1i8 @MAIN_10 -1i8 @MAIN_10 -1i8 @MAIN_10
    SWITCH n
        // CHECK-NEXT: MAIN_7:
        CASE 100
            // CHECK-NEXT: WAIT 100i8
            // CHECK-NEXT: GOTO @MAIN_10
            WAIT 100
            BREAK
        // CHECK-NEXT: MAIN_8:
        CASE 200
            // CHECK-NEXT: WAIT 200i16
            // CHECK-NEXT: GOTO @MAIN_10
            WAIT 200
            BREAK
        // CHECK-NEXT: MAIN_9:
        CASE 50
            // CHECK-NEXT: WAIT 50i8
            // CHECK-NEXT: GOTO @MAIN_10
            WAIT 50
            BREAK
    ENDSWITCH
}

// Using over 7 cases, should generate a SWITCH_CONTINUED!
{
    // CHECK-NEXT: MAIN_10:
    // CHECK-NEXT: SWITCH_START &8 9i8 0i8 @MAIN_20 100i8 @MAIN_11 200i16 @MAIN_12 300i16 @MAIN_13 400i16 @MAIN_14 500i16 @MAIN_15 600i16 @MAIN_16 700i16 @MAIN_17
    // CHECK-NEXT: SWITCH_CONTINUED 800i16 @MAIN_18 900i16 @MAIN_19 -1i8 @MAIN_20 -1i8 @MAIN_20 -1i8 @MAIN_20 -1i8 @MAIN_20 -1i8 @MAIN_20 -1i8 @MAIN_20 -1i8 @MAIN_20
    SWITCH n
        // CHECK-NEXT: MAIN_11:
        CASE 100
            // CHECK-NEXT: WAIT 100i8
            // CHECK-NEXT: GOTO @MAIN_20
            WAIT 100
            BREAK
        // CHECK-NEXT: MAIN_12:
        CASE 200
            // CHECK-NEXT: WAIT 200i16
            // CHECK-NEXT: GOTO @MAIN_20
            WAIT 200
            BREAK
        // CHECK-NEXT: MAIN_13:
        CASE 300
            // CHECK-NEXT: WAIT 300i16
            // CHECK-NEXT: GOTO @MAIN_20
            WAIT 300
            BREAK
        // CHECK-NEXT: MAIN_14:
        CASE 400
            // CHECK-NEXT: WAIT 400i16
            // CHECK-NEXT: GOTO @MAIN_20
            WAIT 400
            BREAK
        // CHECK-NEXT: MAIN_15:
        CASE 500
            // CHECK-NEXT: WAIT 500i16
            // CHECK-NEXT: GOTO @MAIN_20
            WAIT 500
            BREAK
        // CHECK-NEXT: MAIN_16:
        CASE 600
            // CHECK-NEXT: WAIT 600i16
            // CHECK-NEXT: GOTO @MAIN_20
            WAIT 600
            BREAK
        // CHECK-NEXT: MAIN_17:
        CASE 700
            // CHECK-NEXT: WAIT 700i16
            // CHECK-NEXT: GOTO @MAIN_20
            WAIT 700
            BREAK
        // CHECK-NEXT: MAIN_18:
        CASE 800
            // CHECK-NEXT: WAIT 800i16
            // CHECK-NEXT: GOTO @MAIN_20
            WAIT 800
            BREAK
        // CHECK-NEXT: MAIN_19:
        CASE 900
            // CHECK-NEXT: WAIT 900i16
            // CHECK-NEXT: GOTO @MAIN_20
            WAIT 900
            BREAK
    ENDSWITCH
}

// Using cases with the same body
{
    // CHECK-NEXT: MAIN_20:
    // CHECK-NEXT: SWITCH_START &8 3i8 1i8 @MAIN_22 1i8 @MAIN_21 2i8 @MAIN_21 3i8 @MAIN_22 -1i8 @MAIN_23 -1i8 @MAIN_23 -1i8 @MAIN_23 -1i8 @MAIN_23
    SWITCH n
        // CHECK-NEXT: MAIN_21:
        CASE 1
        CASE 2
            // CHECK-NEXT: WAIT 100i8
            // CHECK-NEXT: GOTO @MAIN_23
            WAIT 100
            BREAK
        // CHECK-NEXT: MAIN_22:
        CASE 3
        DEFAULT
            // CHECK-NEXT: WAIT 200i16
            // CHECK-NEXT: GOTO @MAIN_23
            WAIT 200
            BREAK
    ENDSWITCH
}

// CHECK-NEXT: MAIN_23:
// CHECK-NEXT: TERMINATE_THIS_SCRIPT
TERMINATE_THIS_SCRIPT