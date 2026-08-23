// RUN: %dis %gta3sc %s --config=gta3 -fsyntax-only 2>&1 | %verify %s

// expected-no-diagnostics

foo: GOTO foo
bar:
GOTO bar

TERMINATE_THIS_SCRIPT