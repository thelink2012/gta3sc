SCRIPT_START
REQUIRE req5_from_stream1.sc
REQUIRE req6_from_everywhere.sc

PRINT_HELP stream1

VAR_INT stream1_var
CREATE_OBJECT stream1_obj .0 .0 .0 stream1_var

stream1_label1:
GOSUB req5_label1
GOTO stream1_label1

SCRIPT_END