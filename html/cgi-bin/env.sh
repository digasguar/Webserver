#!/bin/bash
printf 'Content-Type: text/plain\n\n'
env | LC_ALL=C sort
echo "CWD=$(pwd)"
