#!/usr/bin/env python3
import os

cookie = os.environ.get('HTTP_COOKIE', '')

# print("Status: 200 OK")
print("Content-Type text/html")
print()


# print(f"<h1>{cookie.lower()}</h1>")


if "user_id=oel_hadr" in cookie.lower():
    print("<h1>Welcome back, oel-hadr!</h1>")
else:
    print("<h1>Please log in first.</h1>")
