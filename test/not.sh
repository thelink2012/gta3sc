#!/bin/sh
# Runs a program and uses exit code 1 as success.
# Any other exit code is a failure (including crashes).
"$@"
if [ $? -eq 1 ]; then
    exit 0
else
    exit 1
fi
