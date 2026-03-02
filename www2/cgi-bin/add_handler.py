#!/usr/bin/python3
import os, sys, base64, cgi

script_dir = os.path.dirname(os.path.abspath(__file__))
sys.path.append(script_dir)
from utils import send_403

def get_session_user():
    cookie_str = os.environ.get('HTTP_COOKIE', '')
    for item in cookie_str.split(';'):
        name, sep, value = item.strip().partition('=')
        if name == 'session_id':
            try:
                return base64.b64decode(value).decode().strip()
            except: return None
    return None

# Use cgi module to parse POST data
form = cgi.FieldStorage()
filename = form.getvalue('filename', '').strip()
content = form.getvalue('content', '')
requested_user = form.getvalue('user', '').strip()
session_user = get_session_user()

if not session_user or session_user != requested_user:
    send_403("Unauthorized: You cannot send mail for another user.")

if not filename:
    print("Status: 400 Bad Request\r\n\r\nFilename required.")
    sys.exit(0)

# Security: Prevent path traversal (e.g., filename="../../etc/passwd")
filename = os.path.basename(filename)
file_path = os.path.join(script_dir, "..", "users", requested_user, filename)

try:
    with open(file_path, 'w') as f:
        f.write(content)
    print("Status: 201 Created")
    print("Content-Type: text/plain\r\n\r\nEmail Created.")
except Exception as e:
    print("Status: 500 Internal Server Error\r\n\r\nError writing file.")