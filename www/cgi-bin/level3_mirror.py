#!/usr/bin/env python3
import sys
import os

def main():
    # 1. Get the expected length
    try:
        content_length = int(os.environ.get('CONTENT_LENGTH', 0))
    except ValueError:
        content_length = 0

    # 2. CGI Headers
    # Use octet-stream to tell the browser/curl this is raw binary
    print("Status: 200 OK")
    print("Content-Type: application/octet-stream")
    print(f"Content-Length: {content_length}")
    print()
    sys.stdout.flush()

    # 3. Mirror the STDIN to STDOUT in raw binary mode
    bytes_processed = 0
    chunk_size = 65536 # 64KB
    
    while bytes_processed < content_length:
        # We use .buffer to avoid Python trying to decode UTF-8
        chunk = sys.stdin.buffer.read(min(chunk_size, content_length - bytes_processed))
        if not chunk:
            break
        sys.stdout.buffer.write(chunk)
        bytes_processed += len(chunk)
    
    sys.stdout.buffer.flush()

if __name__ == "__main__":
    main()
