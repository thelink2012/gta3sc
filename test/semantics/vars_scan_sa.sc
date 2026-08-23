// RUN: %dis %gta3sc %s --config=gtasa --guesser -fsyntax-only 2>&1 | %verify %s
// Also see other tests in vars_scan_3vc.sc

VAR_INT con[XYZ]  // expected-error {{index must be constant}}
VAR_INT zero[0]   // expected-error {{declaring a zero-sized array}}
VAR_INT neg[-1]   // expected-error {{index cannot be negative}}
VAR_INT maxa[256] // expected-error {{arrays are limited to a maximum of 255 elements}}
VAR_INT maxb[255]

TERMINATE_THIS_SCRIPT