// RUN: %dis %gta3sc %s --config=gtasa --guesser --cs -fsyntax-only 2>&1 | %verify %s
SCRIPT_START
{
START_NEW_SCRIPT script_label		// stops in a compilation step where this check doesn't happen
LAUNCH_MISSION subscript.sc			// expected-error {{this command is not allowed in custom scripts}}
LOAD_AND_LAUNCH_MISSION mission.sc	// expected-error {{this command is not allowed in custom scripts}}
REGISTER_STREAMED_SCRIPT stream.sc	// expected-error {{this command is not allowed in custom scripts}}

LOAD_AND_LAUNCH_MISSION_INTERNAL 0	// fine

TERMINATE_THIS_CUSTOM_SCRIPT
}

{
script_label:
TERMINATE_THIS_SCRIPT // stops in a compilation step where this check doesn't happen
}
SCRIPT_END