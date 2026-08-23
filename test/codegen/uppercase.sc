// Tests the codegeneration of expressions.
// RUN: %gta3sc-filecheck %s --config=gta3 -emit-ir2 -o -

// CHECK: PRINT_HELP 'UPPER'
PRINT_HELP upper

TERMINATE_THIS_SCRIPT