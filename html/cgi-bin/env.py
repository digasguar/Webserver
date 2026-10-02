#!/usr/bin/env python3
import os, sys
sys.stdout.write("Content-Type: text/plain\n\n")
for k in sorted(os.environ): sys.stdout.write("%s=%s\n" % (k, os.environ[k]))
sys.stdout.write("CWD=%s\n" % os.getcwd())
