SCRIPT_START
PRINT_HELP stream1

stream1_label1:
IS_BUTTON_PRESSED PAD1 RIGHTSHOCK
GOTO_IF_FALSE stream1_label2
GOTO stream1_label3

stream1_label2:
WAIT 0
GOTO stream1_label1

stream1_label3:
SCRIPT_END