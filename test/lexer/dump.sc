// RUN: %gta3sc-verify %s --config=gta3 -fsyntax-only

DUMP
    FF 00 00 7F
    FF00007F
    F           // expected-error {{hexadecimal tokens must have pairs of two digits}}
    7A7         // expected-error {{hexadecimal tokens must have pairs of two digits}}
    AQ          // expected-error {{invalid hexadecimal token}}
    AFAQ09      // expected-error {{invalid hexadecimal token}}
ENDDUMP

TERMINATE_THIS_SCRIPT
