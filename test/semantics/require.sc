// RUN: %gta3sc-verify %s --config=gta3 -fsyntax-only
REQUIRE req1.sc
TERMINATE_THIS_SCRIPT

// expected-error@req1.sc:1 {{using REQUIRE inside a required file is forbidden}}
