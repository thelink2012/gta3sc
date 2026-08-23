// Regression Test 5
// Ensures the multifile header contains the maximum local variable used in a mission.
//
// # Check SCM Header
// RUN: mkdir "%/T/0005" || echo _
// RUN: %gta3sc %s --config=gtasa --guesser -o "%/T/0005/main.scm"
// RUN: %checksum "%T/0005/main.scm" bf59e9794dfd10dbc1cbb8209c052aff
//
LOAD_AND_LAUNCH_MISSION miss1.sc
LOAD_AND_LAUNCH_MISSION miss2.sc
TERMINATE_THIS_SCRIPT