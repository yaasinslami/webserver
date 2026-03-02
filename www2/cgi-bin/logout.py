#!/usr/bin/python3
print("Status: 302 Found")
print("Location: /login.html")
# Set the cookie to empty and expire it in the past
print("Set-Cookie: session_id=; Expires=Thu, 01 Jan 1970 00:00:00 GMT; Path=/; HttpOnly")
print("\r\n")