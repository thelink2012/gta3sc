// Tests the codegeneration of expressions.
// RUN: %gta3sc %s --config=gta3 -emit-ir2 -o - | %FileCheck %s

// CHECK: PRINT_HELP 'UPPER'
PRINT_HELP upper

TERMINATE_THIS_SCRIPT