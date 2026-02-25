#!/usr/bin/env python3
import sys
import os

# Read metadata from environment variables set by your server
content_type = os.environ.get('CONTENT_TYPE', 'unknown')
content_length = int(os.environ.get('CONTENT_LENGTH', 0))

# Read the raw body from stdin
raw_body = sys.stdin.read(content_length)

sys.stdout.write("Content-Type: text/plain\r\n")
sys.stdout.write("Status: 200 OK\r\n")
sys.stdout.write("\r\n")
sys.stdout.flush()
print("--- CGI MULTIPART TEST ---")
print(f"Content-Type received: {content_type}")
print(f"Content-Length received: {content_length}")
print("-" * 30)
print("RAW BODY RECEIVED:")
print(raw_body)
