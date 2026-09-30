#!/usr/bin/env python3
import sys, os
body = sys.stdin.read()
print("Content-Type: text/plain")
print()
print("REQUEST_METHOD=" + os.environ.get("REQUEST_METHOD", "?"))
print("QUERY_STRING=" + os.environ.get("QUERY_STRING", "?"))
print("BODY_RECEIVED=" + body)
