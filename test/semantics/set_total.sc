// RUN: %dis %gta3sc %s --config=gta3 -fsyntax-only 2>&1 | %verify %s

// Set total once
SET_PROGRESS_TOTAL 1			// expected-error {{must be 0}}
SET_TOTAL_NUMBER_OF_MISSIONS 1	// expected-error {{must be 0}}
SET_COLLECTABLE1_TOTAL 1		// expected-error {{must be 0}}

// Set total again (error)
SET_PROGRESS_TOTAL 0			// expected-error {{multiple times}}
								// expected-note@4 {{previously seen here}}
SET_TOTAL_NUMBER_OF_MISSIONS 0  // expected-error {{multiple times}}
								// expected-note@5 {{previously seen here}}
SET_COLLECTABLE1_TOTAL 0  		// expected-error {{multiple times}}
								// expected-note@6 {{previously seen here}}

TERMINATE_THIS_SCRIPT