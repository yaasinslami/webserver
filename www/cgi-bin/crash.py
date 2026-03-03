import requests # or curl via subprocess
print("Content-Type: text/html\r\n\r\n")
print("<h1>Recursive Step</h1>")

requests.get("http://localhost:8080/cgi-bin/crash.py")