MISSION_START

REQUIRE req4_from_miss1.sc
REQUIRE req6_from_everywhere.sc

PRINT_HELP miss1

VAR_INT miss1_var
CREATE_OBJECT miss1_obj .0 .0 .0 miss1_var

miss1_label1:
GOSUB req4_label1
GOTO miss1_label1

MISSION_END