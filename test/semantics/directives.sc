// RUN: %dis %gta3sc %s --config=gtasa --guesser -fsyntax-only 2>&1 | %verify %s

MISSION_START // expected-error@directives.sc:0 {{cannot use MISION_START in main scripts}}
LAUNCH_MISSION subscript.sc
LOAD_AND_LAUNCH_MISSION miss1.sc
LOAD_AND_LAUNCH_MISSION miss2.sc
LOAD_AND_LAUNCH_MISSION miss3.sc
LOAD_AND_LAUNCH_MISSION miss4.sc
LOAD_AND_LAUNCH_MISSION miss5.sc
REGISTER_STREAMED_SCRIPT stream1 stream1.sc
TERMINATE_THIS_SCRIPT

// expected-error@subscript.sc:0 {{subscript script does not contain MISSION_START}}

// expected-error@miss1.sc:0 {{mission script does not contain MISSION_START}}
// expected-error@miss2.sc:0 {{mission script does not contain MISSION_END}}
// expected-error@miss3.sc:1 {{MISSION_END without a MISSION_START}}
// expected-error@miss3.sc:0 {{mission script does not contain MISSION_START}}
// expected-error@miss4.sc:2 {{MISSION_START must be the first statement in script}}
// expected-error@miss5.sc:2 {{more than one MISSION_START in script}}
// expected-error@miss5.sc:4 {{more than one MISSION_END in script}}

// expected-error@stream1.sc:0 {{streamed script does not contain SCRIPT_START}}
