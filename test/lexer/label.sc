// RUN: %gta3sc-verify %s --config=gta3 -fsyntax-only

// expected-no-diagnostics

foo: GOTO foo
bar:
GOTO bar

TERMINATE_THIS_SCRIPT