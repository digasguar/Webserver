#!/usr/bin/env python3
import os
from urllib.parse import parse_qs

qs = parse_qs(os.environ.get("QUERY_STRING", ""))
try:
    a = float(qs.get("a", ["0"])[0])
    b = float(qs.get("b", ["0"])[0])
    result = a + b
    body = f"<html><body><h1>{a} + {b} = {result}</h1></body></html>"
except ValueError:
    body = "<html><body><h1>Invalid numbers</h1></body></html>"

print("Content-Type: text/html")
print(f"Content-Length: {len(body)}")
print()
print(body, end="")
