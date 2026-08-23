// RUN: %dis %gta3sc %s --config=gta3 -fsyntax-only 2>&1 | %verify %s
GOSUB_FILE gosub1 gosub1.sc
LAUNCH_MISSION subscript.sc
LAUNCH_MISSION gosub1.sc // expected-error {{first seen as extension}}
TERMINATE_THIS_SCRIPT

// expected-error@gta3sc:0 {{subscript.sc was first seen as subscript then as mission}}

// expected-error@gosub1.sc:3 {{script file extension must be .sc}}
// expected-error@gosub1.sc:4 {{script file extension must be .sc}}
// expected-error@gosub1.sc:5 {{too few arguments}}

// expected-warning@subscript.sc:2 {{use of this command outside main or extension files is not well-defined}}
// expected-error@subscript.sc:3 {{use of this command outside main or extension files is not well-defined}}
