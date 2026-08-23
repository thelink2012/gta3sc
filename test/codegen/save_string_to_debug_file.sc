// RUN: %gta3sc %s --config=gtasa --guesser -emit-ir2 -o - | %FileCheck %s

// CHECK: SAVE_STRING_TO_DEBUG_FILE b"THE QUICK BROWN FOX JUMPS OVER THE LAZY DOG"
SAVE_STRING_TO_DEBUG_FILE "The quick brown fox jumps over the lazy dog"

TERMINATE_THIS_SCRIPT