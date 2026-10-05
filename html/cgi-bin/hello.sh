#!/bin/bash
body=$(cat; echo x); body=${body%x}   # keep trailing newlines
printf 'Content-Type: text/plain\n\n'
echo "REQUEST_METHOD=${REQUEST_METHOD-?}"
echo "QUERY_STRING=${QUERY_STRING-?}"
echo "BODY_RECEIVED=$body"
