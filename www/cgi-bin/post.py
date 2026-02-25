#!/usr/bin/env python3
import sys
import os

# 1. Read the body from stdin
# Content-Length tells us exactly how many bytes to read
try:
    content_length = int(os.environ.get('CONTENT_LENGTH', 0))
    body = sys.stdin.read(content_length)
except (ValueError, TypeError):
    body = "No body received or invalid Content-Length"

# 2. Print Headers
print("Content-Type: text/plain\r\n", end='')
print("Status: 200 OK\r\n", end='')
print("\r\n", end='')

# 3. Print Body for verification
print("--- CGI POST VERIFICATION ---")
print(f"Method: {os.environ.get('REQUEST_METHOD', 'N/A')}")
print(f"Content-Length: {content_length}")
print(f"Body Content: {body}")
