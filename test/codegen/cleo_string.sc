// RUN: %gta3sc %s --config=gtasa --guesser --cs -emit-ir2 -o - | %FileCheck %s
SCRIPT_START
{
LVAR_INT ptr1 ptr2
LVAR_TEXT_LABEL16 text16

// CHECK-NEXT: COPY_FILE "PTR1" "PTR2"
COPY_FILE ptr1 ptr2
// CHECK-NEXT: COPY_FILE 0@ 1@
COPY_FILE $ptr1 $ptr2
// CHECK-NEXT: COPY_FILE "Preserve Case" 2@v
COPY_FILE "Preserve Case" $text16

TERMINATE_THIS_CUSTOM_SCRIPT
}
SCRIPT_END