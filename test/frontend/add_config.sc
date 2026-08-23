// Tests the --add-config flag.
// RUN:      %gta3sc %s --config=gta3 -fsyntax-only --add-config=./Inputs/test.xml 2>&1
// RUN: %not %gta3sc %s --config=gta3 -fsyntax-only --add-config=Inputs/test.xml 2>&1 | grep "could not open file" 

TEST_COMMAND 0 0
