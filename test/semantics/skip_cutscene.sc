// RUN: %dis %gta3sc %s --config=gtasa --guesser -fsyntax-only 2>&1 | %verify %s

SKIP_CUTSCENE_END // expected-error {{SKIP_CUTSCENE_END without SKIP_CUTSCENE_START}}

SKIP_CUTSCENE_START
SKIP_CUTSCENE_START // expected-error {{SKIP_CUTSCENE_START inside another SKIP_CUTSCENE_START}}
WAIT 0
SKIP_CUTSCENE_END

SKIP_CUTSCENE_START_INTERNAL label1
label1:

SKIP_CUTSCENE_START
// expected-error@skip_cutscene.sc:0 {{missing SKIP_CUTSCENE_END}}

TERMINATE_THIS_SCRIPT