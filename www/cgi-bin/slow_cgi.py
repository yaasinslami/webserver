#!/usr/bin/env python3
import time
import sys

print("Content-Type: text/plain\r\n\r\n", end="")
sys.stdout.flush()

for i in range(10):
    print(f"Byte {i}")
    sys.stdout.flush()
    time.sleep(2)
