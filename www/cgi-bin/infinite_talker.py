#!/usr/bin/python3
import time
import sys

print("Content-Type: text/plain\r\n\r\n", end="")
while True:
    print("I am still alive and sending data...\n")
    sys.stdout.flush() # Force data out to reset lastActivity
    time.sleep(1)