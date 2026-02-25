#!/usr/bin/env python3
import cgi
import os
import sys

UPLOAD_DIR = './uploads'
if not os.path.exists(UPLOAD_DIR):
    os.makedirs(UPLOAD_DIR)

form = cgi.FieldStorage()

print("Content-Type: text/html\r\n\r\n", end="")

username = form.getvalue('username')
file_item = form['file'] if 'file' in form else None

if file_item is not None and file_item.filename:
    fn = os.path.basename(file_item.filename)
    filepath = os.path.join(UPLOAD_DIR, fn)

    with open(filepath, 'wb') as f:
        f.write(file_item.file.read())
        
    message = f"<h2>Success!</h2><p>User {username} uploaded {fn} to {UPLOAD_DIR}</p>"
else:
    message = "<h2>Error</h2><p>No file was uploaded.</p>"

print(f"<html><body>{message}</body></html>")
