// RUN: %dis %gta3sc %s --config=gta3 -fsyntax-only 2>&1 | %verify %s
REQUIRE req1.sc
TERMINATE_THIS_SCRIPT

// expected-error@req1.sc:1 {{using REQUIRE inside a required file is forbidden}}
