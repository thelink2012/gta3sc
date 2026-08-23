#!/bin/sh
# Verifies that the md5sum of the file $1 equals $2.
md5sum "$1" | grep "$2" >/dev/null || {
    echo "checksum: file $1 checksum is not $2" >&2
    exit 1
}
