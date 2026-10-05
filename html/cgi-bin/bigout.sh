#!/bin/bash
# Bash variables can't hold NUL bytes, so build the 256-byte chunk in a temp file.
chunk=$(mktemp) || exit 1
trap 'rm -f "$chunk"' EXIT

for ((i = 0; i < 256; i++)); do
    printf "\\$(printf '%03o' $(( (i * 7 + 3) & 255 )))"
done > "$chunk"

printf 'Content-Type: application/octet-stream\n\n'
for ((n = 0; n < 5 * 1024 * 4; n++)); do   # 5 MiB
    cat "$chunk"
done
