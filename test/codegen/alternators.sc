// RUN: %gta3sc %s --config=gtasa --guesser -emit-ir2 -o - | %FileCheck %s

{

VAR_INT   gi1 gi2
VAR_FLOAT gf1 gf2

LVAR_INT   ti1 ti2
LVAR_FLOAT tf1 tf2

VAR_TEXT_LABEL   gs1 gs2
VAR_TEXT_LABEL16 gv1 gv2

LVAR_TEXT_LABEL   ts1 ts2
LVAR_TEXT_LABEL16 tv1 tv2

// CHECK-L: NOP
NOP

// CHECK-NEXT-L: SET_VAR_INT &8 1i8
SET_VAR_INT gi1 1

// CHECK-NEXT-L: SET_VAR_INT &8 1i8
SET gi1 1

// CHECK-NEXT-L: SET_VAR_FLOAT &16 0x1.000000p+0f
SET gf1 1.0

// CHECK-NEXT-L: SET_LVAR_INT 0@ 1i8
SET ti1 1

// CHECK-NEXT-L: SET_LVAR_FLOAT 2@ 0x1.000000p+0f
SET tf1 1.0

// CHECK-NEXT-L: SET_VAR_INT_TO_VAR_INT &8 &12
SET gi1 gi2

// CHECK-NEXT-L: SET_LVAR_INT_TO_LVAR_INT 0@ 1@
SET ti1 ti2

// CHECK-NEXT-L: SET_VAR_FLOAT_TO_VAR_FLOAT &16 &20
SET gf1 gf2

// CHECK-NEXT-L: SET_LVAR_FLOAT_TO_LVAR_FLOAT 2@ 3@
SET tf1 tf2

// CHECK-NEXT-L: SET_VAR_FLOAT_TO_LVAR_FLOAT &16 2@
SET gf1 tf1

// CHECK-NEXT-L: SET_LVAR_FLOAT_TO_VAR_FLOAT 2@ &16
SET tf1 gf1

// CHECK-NEXT-L: SET_VAR_INT_TO_LVAR_INT &8 0@
SET gi1 ti1

// CHECK-NEXT-L: SET_LVAR_INT_TO_VAR_INT 0@ &8
SET ti1 gi1

// CHECK-NEXT-L: SET_VAR_INT_TO_CONSTANT &8 285i16
SET gi1 SWAT

// CHECK-NEXT-L: SET_LVAR_INT_TO_CONSTANT 0@ 285i16
SET ti1 SWAT

// CHECK-NEXT-L: SET_VAR_TEXT_LABEL s&24 'LABEL'
SET gs1 LABEL
// CHECK-NEXT-L: SET_VAR_TEXT_LABEL s&24 s&32
SET gs1 $gs2
// CHECK-NEXT-L: SET_VAR_TEXT_LABEL s&24 6@s
SET gs1 $ts2

// CHECK-NEXT-L: SET_LVAR_TEXT_LABEL 4@s 'LABEL'
SET ts1 LABEL
// CHECK-NEXT-L: SET_LVAR_TEXT_LABEL 4@s 6@s
SET ts1 $ts2
// CHECK-NEXT-L: SET_LVAR_TEXT_LABEL 4@s s&32
SET ts1 $gs2

// CHECK-NEXT-L: SET_VAR_TEXT_LABEL16 v&40 "LABEL"
SET gv1 LABEL
// CHECK-NEXT-L: SET_VAR_TEXT_LABEL16 v&40 v&56
SET gv1 $gv2
// CHECK-NEXT-L: SET_VAR_TEXT_LABEL16 v&40 12@v
SET gv1 $tv2

// CHECK-NEXT-L: SET_LVAR_TEXT_LABEL16 8@v "LABEL"
SET tv1 LABEL
// CHECK-NEXT-L: SET_LVAR_TEXT_LABEL16 8@v 12@v
SET tv1 $tv2
// CHECK-NEXT-L: SET_LVAR_TEXT_LABEL16 8@v v&56
SET tv1 $gv2

// CHECK-NEXT-L: CSET_VAR_INT_TO_VAR_FLOAT &8 &16
CSET gi1 gf1

// CHECK-NEXT-L: CSET_VAR_FLOAT_TO_VAR_INT &16 &8
CSET gf1 gi1

// CHECK-NEXT-L: CSET_LVAR_INT_TO_VAR_FLOAT 0@ &16
CSET ti1 gf1

// CHECK-NEXT-L: CSET_LVAR_FLOAT_TO_VAR_INT 2@ &8
CSET tf1 gi1

// CHECK-NEXT-L: CSET_VAR_INT_TO_LVAR_FLOAT &8 2@
CSET gi1 tf1

// CHECK-NEXT-L: CSET_VAR_FLOAT_TO_LVAR_INT &16 0@
CSET gf1 ti1

// CHECK-NEXT-L: CSET_LVAR_INT_TO_LVAR_FLOAT 0@ 2@
CSET ti1 tf1

// CHECK-NEXT-L: CSET_LVAR_FLOAT_TO_LVAR_INT 2@ 0@
CSET tf1 ti1

}

TERMINATE_THIS_SCRIPT