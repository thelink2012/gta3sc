// RUN: %dis %gta3sc %s --config=gtavc -fsyntax-only 2>&1 | %verify %s
VAR_INT car char

car = char

IS_CHAR_IN_CAR char 0 // expected-error {{expected variable of type CHAR but got NONE}}

CREATE_CAR 0 .0 .0 .0 car
CREATE_CHAR 0 0 .0 .0 .0 char

car = char	// expected-error {{assignment of variable of type CHAR into one of type CAR}}
car = char  // fine now, car is of type CHAR since the assignment above

IS_CHAR_IN_CAR 0 char // expected-error {{expected variable of type CAR but got CHAR}}
IS_CHAR_IN_CAR 0 0

CREATE_CAR 0 .0 .0 .0 char // expected-error {{variable has already been used to create a entity of type CHAR}}

TERMINATE_THIS_SCRIPT