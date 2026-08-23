// Regression Test 1
// Checks whether a mission script which is the first to declare global vars
// (i.e. main script has no globals) properly starts at &8 offset.
//
// RUN: %gta3sc-filecheck %s --config=gta3 -emit-ir2 -o -
//
LOAD_AND_LAUNCH_MISSION mission.sc
TERMINATE_THIS_SCRIPT

// mission.sc
// CHECK:      SCRIPT_NAME 'MISS1'
// CHECK-NEXT: SET_VAR_INT &8 2
