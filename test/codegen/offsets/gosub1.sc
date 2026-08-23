gosub1:
PRINT_HELP gosub1

gosub1_label1:
IS_BUTTON_PRESSED PAD1 RIGHTSHOCK
GOTO_IF_FALSE gosub1_label2
GOTO gosub1_label3

gosub1_label2:
WAIT 0
GOTO gosub1_label1

gosub1_label3:
RETURN
