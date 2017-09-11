// RUN: %gta3sc %s --config=gta3 --cs -emit-ir2 -o - | %FileCheck %s
// RUN: %gta3sc %s --config=gtavc --cs -emit-ir2 -o - | %FileCheck %s
SCRIPT_START
{
LVAR_INT ptr1 ptr2
//LVAR_TEXT_LABEL16 text16

// CHECK-NEXT-L: COPY_FILE "PTR1" "PTR2"
COPY_FILE ptr1 ptr2
// CHECK-NEXT-L: COPY_FILE 0@ 1@
COPY_FILE $ptr1 $ptr2
// CHECK-NEXT-L: COPY_FILE "Preserve Case" 0@
COPY_FILE "Preserve Case" $ptr1 // test changed from SA

TERMINATE_THIS_CUSTOM_SCRIPT
}
SCRIPT_END
