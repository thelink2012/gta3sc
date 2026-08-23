// RUN: %dis %gta3sc %s --config=gta3 -fsyntax-only 2>&1 | %verify %s
SCRIPT_NAME name
LOAD_AND_LAUNCH_MISSION miss1.sc
TERMINATE_THIS_SCRIPT

// expected-error@miss1.sc:2 {{duplicate script name}}
// expected-note@script_name.sc:2 {{previously used here}}
