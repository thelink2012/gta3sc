// RUN: %gta3sc-verify %s --config=gta3 -fsyntax-only -Wexpect-var
// RUN: %gta3sc-verify %s --config=gtavc -fsyntax-only -Wexpect-var

// expected-warning@gta3sc:0 {{expected variable scplayer to exist}}
// expected-warning@gta3sc:0 {{expected variable player to exist}}
// expected-warning@gta3sc:0 {{expected variable flag_player_on_mission to exist}}

TERMINATE_THIS_SCRIPT