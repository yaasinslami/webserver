#!/usr/bin/python3
import os, sys, base64
# import send_403 from utils.py in ../cgi-bin
sys.path.append(os.path.join(os.getcwd(), "../cgi-bin"))
from utils import send_403

def get_cookies():
    cookie_str = os.environ.get('HTTP_COOKIE', '')
    if not cookie_str:
        return {}
    
    cookies = {}
    # Split by semicolon to get individual cookies
    for item in cookie_str.split(';'):
        # use partition instead of split to handle '=' inside values (like Base64)
        name, sep, value = item.strip().partition('=')
        if sep:
            cookies[name] = value
    return cookies

# 1. Session Validation
cookies = get_cookies()
session_id = cookies.get('session_id', '')
query_user = os.environ.get('QUERY_STRING', '').split('user=')[-1]

# Decode session to check if it matches the requested user
try:
    decoded_user = base64.b64decode(session_id).decode() if session_id else ""
except:
    decoded_user = ""

if not session_id or decoded_user != query_user:
    send_403("Your session is invalid. Please log in again to view your inbox.")
    sys.exit(0)


user_dir = os.path.join("../users", query_user)
emails = os.listdir(user_dir) if os.path.exists(user_dir) else []

print("Content-Type: text/html\r\n\r\n")
print(f"""
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Gmail Clone - {query_user}</title>
    <link rel="stylesheet" href="/assets/style.css">
</head>
<body>
    <nav>
        <div class="logo">GMAIL</div>
        <div class="nav-item active">📥 Inbox ({len(emails)})</div>
        <div class="nav-item" onclick="location.href='/cgi-bin/logout.py'"> ➜] Logout</div>
    </nav>
    <main>
        <div class="header">
            <h1>Welcome, {query_user}</h1>
            <button class="btn-new" onclick="openAddModal('{query_user}')">+ New Email</button>
        </div>
        <div class="grid">
""")

for email in emails:
    file_path = os.path.join(user_dir, email)
    sender = "Unknown"
    snippet = "No content..."
    
    try:
        with open(file_path, 'r') as f:
            content = f.read()
            if '|' in content:
                sender, snippet = content.split('|', 1)
            else:
                snippet = content
    except:
        pass

    preview = (snippet[:50] + '...') if len(snippet) > 50 else snippet

    print(f"""
    <div class="email-row" id="card-{email}">
        <div class="email-main-info" onclick="window.location.href='/users/{query_user}/{email}'">
            <span class="email-sender">{sender}</span>
            <span class="email-subject">{email}</span>
            <span class="email-snippet">- {preview}</span>
        </div>
        <div class="email-actions">
            <button class="btn-row-del" onclick="event.stopPropagation(); openDeleteModal('{email}', '{query_user}')">
                🗑️
            </button>
        </div>
    </div>
    """)

print(f"""
        </div>
    </main>

    <div id="deleteModal" class="modal-overlay">
        <div class="modal-content">
            <div class="modal-icon">⚠️</div>
            <h2>Confirm Deletion</h2>
            <p>Are you sure you want to delete <span id="modalFileName" style="color: #3b82f6; font-weight: 600;"></span>? This action cannot be undone.</p>
            <div class="modal-actions">
                <button class="btn-secondary" onclick="closeModal()">Cancel</button>
                <button id="confirmDeleteBtn" class="btn-danger">Delete Email</button>
            </div>
        </div>
    </div>

    <div id="addModal" class="modal-overlay">
    <div class="modal-content">
            <h2>Compose New Email</h2>
            <input type="text" id="newSender" placeholder="From (e.g. Google, GitHub, 1337)" class="modal-input">
            <input type="text" id="newFileName" placeholder="Subject" class="modal-input">
            <textarea id="newFileContent" placeholder="Email Content..." class="modal-area"></textarea>
            <div class="modal-actions">
                <button class="btn-secondary" onclick="closeAddModal()">Cancel</button>
                <button onclick="handleAddEmail()" class="btn-primary-action">Send Email</button>
            </div>
        </div>
    </div>

    <script src="/assets/inbox.js"></script>
</body>
</html>
""")
