#!/usr/bin/python3
import sys

def send_403(message="Your session has expired or is invalid. Please re-authenticate."):
    print("Status: 403 Forbidden")
    print("Content-Type: text/html\r\n\r\n")
    print(f"""
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <style>
            body {{ background: #0f172a; color: white; font-family: 'Inter', sans-serif; display: flex; align-items: center; justify-content: center; height: 100vh; margin: 0; }}
            .error-container {{ text-align: center; padding: 3rem; background: rgba(255, 255, 255, 0.03); backdrop-filter: blur(10px); border: 1px solid rgba(255, 255, 255, 0.1); border-radius: 1.5rem; max-width: 400px; }}
            h1 {{ font-size: 4rem; margin: 0; background: linear-gradient(to right, #3b82f6, #60a5fa); -webkit-background-clip: text; -webkit-text-fill-color: transparent; font-weight: 800; }}
            p {{ opacity: 0.6; line-height: 1.6; margin: 1rem 0 2rem 0; }}
            .btn-home {{ display: inline-block; background: #3b82f6; color: white; text-decoration: none; padding: 0.8rem 2rem; border-radius: 0.5rem; font-weight: 600; transition: transform 0.2s; }}
            .btn-home:hover {{ background: #2563eb; transform: translateY(-2px); }}
        </style>
    </head>
    <body>
        <div class="error-container">
            <h1>403</h1>
            <div style="font-weight: 600; letter-spacing: 0.05em; margin-bottom: 0.5rem;">ACCESS DENIED</div>
            <p>{message}</p>
            <a href="/login.html" class="btn-home">Return to Login</a>
        </div>
    </body>
    </html>
    """)
    sys.exit(0)