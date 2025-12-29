import socket
import time

# Configuration
HOST = '127.0.0.1'
PORT = 8080 # Change to your server port

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((HOST, PORT))

# 1. Send Partial Headers
print("Sending Headers...")
request_part_1 = b"POST /upload HTTP/1.1\r\nHost: localhost\r\nContent-Length: 10\r\n\r\n"
s.send(request_part_1)

# 2. SLEEP - This forces your server to return to poll()
print("Sleeping for 2 seconds...")
time.sleep(2)

# 3. Send Body
print("Sending Body...")
request_part_2 = b"1234567890"
s.send(request_part_2)

# 4. Wait for response
response = s.recv(4096)
print("\nResponse from server:")
print(response.decode())

s.close()