// Tests the semantics of expressions
// RUN: %dis %gta3sc %s --config=gtasa --guesser -fsyntax-only 2>&1 | %verify %s

VAR_INT   i j k a[10]
VAR_FLOAT x y z

// Test non-commutative operations
i = 2 * i
i = i * 2
i = 2 + i
i = i + 2
i = 2 / i   // expected-error {{cannot do VAR1 = THING / VAR1}}
i = i / 2
i = 2 - i   // expected-error {{cannot do VAR1 = THING - VAR1}}
i = i - 2
x = 2.0 +@ x  // expected-error {{cannot do VAR1 = THING +@ VAR1}} 
x = x +@ 2.0
x = 2.0 -@ x  // expected-error {{cannot do VAR1 = THING -@ VAR1}} 
x = x -@ 2.0
// ...except that those are valid...
x = x - x
x = x / x
x = x +@ x
x = x -@ x

// Increment/Decrement only works with integers
++i
++x 	// expected-error {{could not match alternative}}
--nope	// expected-error {{could not match alternative}}
--a[0]

// Expressions are disallowed in condition lists
IF i = i + 2 // expected-error {{expression not allowed in this context}}
OR i += 2	 // expected-error {{expression not allowed in this context}}
OR i =# x	 // expected-error {{expression not allowed in this context}}
OR ++i	     // expected-error {{expression not allowed in this context}}
OR i = 2	 // fine
    NOP
ENDIF

TERMINATE_THIS_SCRIPT