// RUN: %dis %gta3sc %s --config=gta3 -fsyntax-only -Wexpect-var 2>&1 | %verify %s

VAR_INT player scplayer flag_player_on_mission

// expected-warning-re@gta3sc:0 {{expected variable scplayer to have index \d+ but it has index \d+}}
// expected-warning-re@gta3sc:0 {{expected variable player to have index \d+ but it has index \d+}}
// expected-warning-re@gta3sc:0 {{expected variable flag_player_on_mission to have index \d+ but it has index \d+}}

TERMINATE_THIS_SCRIPT