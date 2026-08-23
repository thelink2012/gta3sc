// RUN: %gta3sc-filecheck %s --config=gtasa --guesser --cs -emit-ir2 -o -
SCRIPT_START
{
LVAR_INT i r
LVAR_FLOAT f
LVAR_TEXT_LABEL text8
LVAR_TEXT_LABEL16 text16

// CHECK-NEXT: CALL_FUNCTION_RETURN 0i8 4i8 4i8 0@ 2@ "TEXT8" "TEXT16" 1@
CALL_FUNCTION_RETURN 0 4 4 i f text8 text16 r

// CHECK-NEXT: CALL_FUNCTION_RETURN 0i8 4i8 4i8 0@ 2@ 3@s 5@v 1@
CALL_FUNCTION_RETURN 0 4 4 i f $text8 $text16 r

// CHECK-NEXT: CALL_FUNCTION_RETURN 0i8 1i8 1i8 "Test Case" 1@
CALL_FUNCTION_RETURN 0 1 1 "Test Case" r

// NOTE: Using a label or text label variable in the last argument of this family of commands is ill-formed!
// CALL_FUNCTION_RETURN 0 0 0 text8

TERMINATE_THIS_CUSTOM_SCRIPT
}
SCRIPT_END