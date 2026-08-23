#!/bin/sh
# Runs a program and discards the result code, unless the program crashed.
"$@"
code=$?
if [ $code -gt 1 ]; then
    exit $code
else
    exit 0
fi
