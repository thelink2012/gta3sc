// RUN: %gta3sc-filecheck %s --config=gtasa --guesser --cs -emit-ir2 -o -
SCRIPT_START
{
LVAR_INT a
LVAR_TEXT_LABEL text
LVAR_INT i
LVAR_FLOAT f

// CHECK: CLEO_CALL %MAIN_1 2i8 5i8 5i8 0@
CLEO_CALL sum 0 5 5 a

// /////CHECK: CLEO_CALL %MAIN_1 2i8 "TEXT" 1@s 0@
//CLEO_CALL sum 0 text $text a	// sums two pointers

// /////CHECK: CLEO_CALL %MAIN_1 2i8 "Test Case" 1@s 0@
//CLEO_CALL sum 0 "Test Case" $text a	// sums two pointers

// CHECK: CLEO_CALL %MAIN_2 0i8 3@ 4@
CLEO_CALL test_110 0 i f

TERMINATE_THIS_CUSTOM_SCRIPT
}

{
	sum:
	LVAR_INT a b c
	c = a + b
	// CHECK: CLEO_RETURN 1i8 2@
	CLEO_RETURN 0 c
}

{
	test_110: // regression test for #110
	LVAR_INT a
    LVAR_FLOAT b
    // CHECK: CLEO_RETURN 2i8 0@ 1@
    CLEO_RETURN 0 a b
}

SCRIPT_END