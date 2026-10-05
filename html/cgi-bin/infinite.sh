#!/bin/bash
# Intended behaviour of infinite.py: spin forever (the original is actually a
# SyntaxError: missing colon after while(1), so Python exits with status 1).
while :; do
    a=$b
done
