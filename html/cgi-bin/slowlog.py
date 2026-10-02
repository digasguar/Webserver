#!/usr/bin/env python3
import time
open("/tmp/webserv_slowlog.txt", "a").write("run\n")
time.sleep(1.0)
print("Content-Type: text/plain\n")
print("done")
