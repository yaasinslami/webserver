#!/usr/bin/env python3
import os

# CGI requires headers first
print("Status: 200 OK")
print("Content-Type: text/plain")
print("Custom-Header: Webserv-Test")
print() # The empty line that separates headers from body

print("CGI Execution: SUCCESS")
print("------------------------")
print("Environment Variables received:")
for key, value in os.environ.items():
    print(f"{key}={value}")
