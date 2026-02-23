#!/usr/bin/env python3
import sys
import os

# Headers
print("Status: 200 OK")
print("Content-Type: text/plain")
print()

# Logic
content_length = int(os.environ.get('CONTENT_LENGTH', 0))

print(f"content_length : {content_length}", file=sys.stderr)

if content_length > 0:
    # We read exactly what the server promised in the environment
    body = sys.stdin.read(content_length)
    print(f"CGI received: {body}")
else:
    print("Error: No body received or CONTENT_LENGTH is 0")

