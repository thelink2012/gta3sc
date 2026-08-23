// RUN: %dis %gta3sc %s --config=gtasa --guesser -fsyntax-only 2>&1 | %verify %s

VAR_TEXT_LABEL text8
VAR_TEXT_LABEL16 text16

PRINT_HELP abc
PRINT_HELP _abc
PRINT_HELP semi: // expected-error {{invalid identifier}}

LAUNCH_MISSION _a.sc
LAUNCH_MISSION 4x4.sc // actually a tokenizer test

CUSTOM_PLATE_FOR_NEXT_CAR 0 __abc___

text8 = __abc 
text16 = __abc

TERMINATE_THIS_SCRIPT