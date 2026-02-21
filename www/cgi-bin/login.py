#!/usr/bin/env python3
print("Status: 200 OK")
# This header tells the browser to remember the user
print("Set-Cookie: user_id=oel-hadr; Max-Age=3600; HttpOnly")
print("Content-Type: text/html")
print()
print("<h1>Logged in!</h1>")
