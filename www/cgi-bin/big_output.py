#!/usr/bin/env python3
import sys

# CGI Headers
print("Status: 200 OK")
print("Content-Type: text/plain")
print()

# Generate 3MB of text
for i in range(500000):
    sys.stdout.write(f"Line {i:06}: This is a stress test for the pipe capacity of my webserv.\n")
