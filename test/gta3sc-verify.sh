#!/bin/sh
# Runs gta3sc and redirects the output to the verify script.
# $1 = verify script, $2 = compiler, $3 = source, rest = compiler args
set -eu

verify=${1:?}
compiler=${2:?}
src=${3:?}
shift 3

tmp=$(mktemp) || exit 1
trap 'rm -f -- "$tmp"' EXIT

code=0
"$compiler" -Wno-expect-var "$src" "$@" >"$tmp" 2>&1 || code=$?
if [ "$code" -gt 1 ]; then
    cat "$tmp" >&2
    exit "$code"
fi

"$verify" "$src" <"$tmp"
