#!/usr/bin/python3
import os, sys, base64
from urllib.parse import unquote

# Ensure utils is findable
script_dir = os.path.dirname(os.path.abspath(__file__))
sys.path.append(script_dir)
from utils import send_403

def get_session_user():
    cookie_str = os.environ.get('HTTP_COOKIE', '')
    for item in cookie_str.split(';'):
        name, sep, value = item.strip().partition('=')
        if name == 'session_id':
            try:
                # Use .strip() here because base64 decoding sometimes adds whitespace
                return base64.b64decode(value).decode().strip()
            except:
                return None
    return None

session_user = get_session_user()
query_params = os.environ.get('QUERY_STRING', '')
params = dict(qc.split('=') for qc in query_params.split('&') if '=' in qc)

filename = unquote(params.get('filename', '')).strip()
requested_user = unquote(params.get('user', '')).strip()

# DEBUG: Check your server terminal to see if these match exactly
sys.stderr.write(f"DEBUG: Attempting to delete: '{filename}' for user: '{requested_user} as the requester '{session_user}'\n")

if not session_user or session_user != requested_user:
    # If this triggers, your session cookie doesn't match the URL user
    send_403(f"Security Alert: '{session_user}' cannot delete emails for '{requested_user}'")
    sys.exit(0)

# 2. FILE REMOVAL
script_dir = os.path.dirname(os.path.abspath(__file__))
file_path = os.path.join(script_dir, "..", "users", requested_user, filename)

if os.path.exists(file_path):
    try:
        os.remove(file_path)
        print("Status: 200 OK")
        print("Content-Type: text/plain\r\n\r\n")
        print("Successfully deleted.")
    except Exception as e:
        print("Status: 500 Internal Server Error\r\n\r\n")
else:
    print("Status: 404 Not Found\r\n\r\n")