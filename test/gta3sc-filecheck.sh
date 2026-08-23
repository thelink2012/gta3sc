#!/bin/sh
# Runs gta3sc and redirects the output to filecheck.
# $1 = filecheck, $2 = compiler, $3 = source, rest = compiler args
set -eu

filecheck=${1:?}
compiler=${2:?}
src=${3:?}
shift 3

"$compiler" -Wno-expect-var "$src" "$@" | "$filecheck" "$src"
