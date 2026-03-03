#!/usr/bin/python3
import sys, os
import base64
from datetime import datetime, timedelta

# 1. Parse POST Data
content_length = int(os.environ.get('CONTENT_LENGTH', 0))
post_data = sys.stdin.read(content_length)

user, pwd = "", ""
try:
    # Parsing key=value pairs from the body
    pairs = dict(item.split("=") for item in post_data.split("&"))
    user = pairs.get("user", "")
    pwd = pairs.get("pass", "")
except:
    pass

# 2. Validate against "Database"
found = False
db_path = os.path.join(os.getcwd(), "../users.db")
if os.path.exists(db_path):
    with open(db_path, "r") as f:
        for line in f:
            if f"{user}:{pwd}" == line.strip():
                found = True
                break

# 3. Handle Response
if found:
    # Generate Session Token (Simple Base64 for demo)
    session_token = base64.b64encode(user.encode()).decode()
    
    # Calculate Expiry (5 minutes)
    expire_time = (datetime.utcnow() + timedelta(minutes=5)).strftime("%a, %d %b %Y %H:%M:%S GMT")
    
    # SUCCESS: 302 Redirect + Set-Cookie
    print("Status: 302 Found")
    print(f"Location: /inbox/view.py?user={user}")
    print(f"Set-Cookie: session_id={session_token}; Expires={expire_time}; Path=/; HttpOnly")
    print("\r\n") # End of Headers
else:
    # FAIL: 401 Unauthorized with Modern
    print("Status: 401 Unauthorized")
    print("Content-Type: text/html\r\n\r\n")
    print(f"""
    <!DOCTYPE html>
    <html>
    <head>
        <style>
            body {{ background: #0f172a; color: white; font-family: sans-serif; display: flex; 
                    flex-direction: column; align-items: center; justify-content: center; height: 100vh; margin: 0; }}
            .error-card {{ background: rgba(239, 68, 68, 0.1); border: 1px solid #ef4444; 
                         padding: 2rem; border-radius: 1rem; text-align: center; backdrop-filter: blur(10px); }}
            a {{ color: #3b82f6; text-decoration: none; margin-top: 1rem; display: inline-block; }}
            .icon {{ font-size: 3rem; margin-bottom: 1rem; }}
        </style>
    </head>
    <body>
        <div class="error-card">
            <h2>Authentication Failed</h2>
            <p>The username or password provided for <b>{user}</b> is incorrect.</p>
            <a href="/login.html"> Try Again </a>
        </div>
    </body>
    </html>
    """)