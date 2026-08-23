// RUN: %gta3sc-verify %s --config=gtasa --guesser -fsyntax-only

#ifdef  // expected-error {{wrong number of tokens}}
#ifndef // expected-error {{wrong number of tokens}}
#endif  // expected-error {{#endif without #ifdef}}

#ifdef ABC DEF // expected-error {{wrong number of tokens}}
#endif

#ifndef ABC DEF // expected-error {{wrong number of tokens}}
#endif

#ifdef ABC
#endif ABC // expected-error {{too many tokens}}

#else ABC // expected-error {{too many tokens}}
          // expected-error@-1 {{#else without #ifdef}}

#something // expected-error {{unknown}}

// empty is valid
#
#
#
