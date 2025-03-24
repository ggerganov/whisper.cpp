import http.server
import socketserver
import os
from pathlib import Path

SCRIPT_DIR = Path(__file__).parent.absolute()
DIRECTORY = os.path.join(SCRIPT_DIR, "../build-em/bin")
DIRECTORY = os.path.abspath(DIRECTORY)

class CustomHTTPRequestHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=DIRECTORY, **kwargs)

    def do_GET(self):
        # If requesting a worker file from any subdirectory
        if '.worker.js' in self.path:
            worker_file = os.path.basename(self.path)
            worker_path = os.path.join(DIRECTORY, worker_file)

            if os.path.exists(worker_path):
                self.path = '/' + worker_file

        return super().do_GET()

    def end_headers(self):
        # Add required headers for SharedArrayBuffer
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        self.send_header("Access-Control-Allow-Origin", "*");
        super().end_headers()

PORT = 8000

with socketserver.TCPServer(("", PORT), CustomHTTPRequestHandler) as httpd:
    print(f"Serving directory '{DIRECTORY}' at http://localhost:{PORT}")
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nServer stopped.")
