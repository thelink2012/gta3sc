// RUN: %gta3sc-filecheck %s --config=gtavc -moptimize-zero -emit-ir2 -o -

// CHECK: SET_TIME_SCALE 0i8
SET_TIME_SCALE 0.0

TERMINATE_THIS_SCRIPT