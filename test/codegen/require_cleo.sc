// RUN: %gta3sc-filecheck %s --config=gtasa --guesser -emit-ir2 -o - --cs -D CS
// RUN: %gta3sc-filecheck %s --config=gtasa --guesser -emit-ir2 -o - --cm -D CM
#ifdef CS
SCRIPT_START
#else
MISSION_START
#endif
{
REQUIRE req1.sc
PRINT_HELP top
}
#ifdef CS
SCRIPT_END
#else
MISSION_END
#endif

// CHECK-NEXT: PRINT_HELP 'TOP'
// CHECK-NEXT:   TERMINATE_THIS_{{(CUSTOM_)?}}SCRIPT
// CHECK-NEXT: PRINT_HELP 'REQ1'
