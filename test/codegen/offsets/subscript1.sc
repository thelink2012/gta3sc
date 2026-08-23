MISSION_START
PRINT_HELP subsc1

subsc1_label1:
IS_BUTTON_PRESSED PAD1 RIGHTSHOCK
GOTO_IF_FALSE subsc1_label2
GOTO subsc1_label3

subsc1_label2:
WAIT 0
GOTO subsc1_label1

subsc1_label3:
MISSION_END