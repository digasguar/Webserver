#!/usr/bin/env python3
import sys
sys.stdout.write("Content-Type: application/octet-stream\n\n")
sys.stdout.flush()
sys.stdout.buffer.write(bytes((i*7+3) & 0xff for i in range(256))*(5*1024*4))   # 5 MiB
sys.stdout.buffer.flush()
