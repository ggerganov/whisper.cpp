from fastapi import FastAPI, WebSocket, WebSocketDisconnect
from fastapi.responses import HTMLResponse
from fastapi.middleware.cors import CORSMiddleware
from fastapi.staticfiles import StaticFiles
import uvicorn
import asyncio
import argparse
import logging as log
# get API/methods from parent dir CLI app
import sys
sys.path.append('../') 
from audio_sender import *

app = FastAPI()
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],  # can alter with time
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)
ws = None

app.mount("/transcribe", StaticFiles(directory="static", html=True), name="static")

@app.websocket("/ws/mictranscribe")
async def transcription_endpoint(websocket: WebSocket):
    global ws
    global args
    await websocket.accept()
    ws = websocket
    log.info("Transcription websocked called; about to start eventloop")
    await main("mic", args.chunk_size, args.server_address, args.delay, printwebsocket)

def printwebsocket(msg = "", end = '\n', flush = False):
    asyncio.get_event_loop().create_task(ws.send_text(msg + end))

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Simple audio-sending gRPC websocket client for audio transcription")
    parser.add_argument("-c", "--chunk-size", dest="chunk_size", help="Size of each chunk (in samples) to send to the server", type=int, default=16000)
    parser.add_argument("-d", "--delay", dest="delay", help="Delay in seconds (but can pass float for ms) between each chunk send", type=float, default=0)
    parser.add_argument("-s", "--server-address", dest="server_address", help="Server address (host:port) of gRPC transcription server", default="localhost:50051")
    parser.add_argument("-v", "--verbose", dest="verbose", help="Verbose/debug logging mode", type=bool, default=False)
    parser.add_argument("-o", "--host", dest="host", help="Host interface web app is listening on", default="0.0.0.0")
    parser.add_argument("-p", "--port", dest="port", help="Port web app is listening on", type=int, default=8000)
    args = parser.parse_args()
    log.basicConfig(level=log.DEBUG if args.verbose else log.INFO, format='%(asctime)s %(message)s')
    log.info(f"=====> Access application at: http://{args.host}:{args.port}/transcribe <=====")
    uvicorn.run(app, host=args.host, port=args.port)