MISSION_START
REQUIRE req3_from_subscript.sc
REQUIRE req6_from_everywhere.sc
REQUIRE req7_from_subscript.sc

PRINT_HELP subsc

VAR_INT subscript_var
CREATE_OBJECT subscript_obj .0 .0 .0 subscript_var

subscript_label1:
GOTO subscript_label1

MISSION_END