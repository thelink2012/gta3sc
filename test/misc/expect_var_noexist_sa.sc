// RUN: %dis %gta3sc %s --config=gtasa --guesser -fsyntax-only -Wexpect-var 2>&1 | %verify %s

// expected-warning@gta3sc:0 {{expected variable scplayer to exist}}
// expected-warning@gta3sc:0 {{expected variable player to exist}}
// expected-warning@gta3sc:0 {{expected variable flag_player_on_mission to exist}}
// expected-warning@gta3sc:0 {{expected variable player_group to exist}}

TERMINATE_THIS_SCRIPT