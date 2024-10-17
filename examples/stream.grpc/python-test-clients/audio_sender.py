import wave
import asyncio
import threading
import logging as log
import argparse
import datetime
import pyaudio
from difflib import SequenceMatcher
import re
import sys

import grpc
import transcription_pb2
import transcription_pb2_grpc

LINE_CLEAR = '\x1b[2K' # <-- ANSI sequence
END_PUNCT_RE = re.compile(r"[?.!]$")

# Performs a bidi-streaming call
async def audio_transcription(stub: transcription_pb2_grpc.AudioTranscriptionStub, async_byte_generator, printfunc) -> None:
    call = stub.TranscribeAudio()
    asyncio.get_event_loop().create_task(send_audio(call, async_byte_generator)) # submit audio async from reading transcript
    log.info("** Now waiting for responses **")
    last_response = None
    two_prior_response = None
    while (True):
        response = await call.read()
        if print_utterance(response, last_response, two_prior_response, printfunc):
            two_prior_response = last_response
            last_response = response

def print_utterance(response, last_response, two_prior_response, printfunc) -> bool:
    msg = f"  [{response.seq_num:03d} {response.start_time.ToDatetime().strftime('%H:%M:%S.%f')[:-3]}{'→' if printfunc != print or sys.stdout.encoding.lower().startswith('utf') else '->'}{response.end_time.ToDatetime().strftime('%H:%M:%S.%f')[:-3]}] - {response.transcription}"
    if last_response is None:
        printfunc(msg, flush=True)
    else:
        if response.transcription.strip().upper() in ["[BLANK AUDIO]", "[ SILENCE ]", "[BLANK_AUDIO]", "[SILENCE]"]:
            return False
        is_intersecting_time = response.start_time.ToDatetime() <  last_response.end_time.ToDatetime()
        if response.transcription == last_response.transcription \
            or (two_prior_response is not None and response.transcription == two_prior_response.transcription) \
            or re.sub(END_PUNCT_RE, "", response.transcription).lower() in re.sub(END_PUNCT_RE, "", last_response.transcription).lower():
            return False
        sim_ratio = SequenceMatcher(None, response.transcription, last_response.transcription).ratio()
        if is_intersecting_time and sim_ratio > 0.8:    # almost identical, and first is usually best
            return False      
        # Correct previous translation if more context has yielded a more complete translation
        if is_intersecting_time and len(response.transcription) > len(last_response.transcription) and (
                sim_ratio > 0.5 or last_response.transcription in response.transcription
        ):
            printfunc(end="\r")
            printfunc(end=LINE_CLEAR, flush=True)
        else:
            printfunc(flush=True)
        printfunc(msg, end="", flush=True)
    return True

async def send_audio(call, async_byte_generator):
    async for mybytes in async_byte_generator:
        request = transcription_pb2.AudioSegmentRequest(audio_data=mybytes)
        request.send_time.FromDatetime(datetime.datetime.now())
        await call.write(request)

async def get_wav_audio_bytes(wav_file: str, chunk_size: int = 16000, delay: float = 0) -> bytes:
    wav = wave.open(wav_file, 'r')
    while True:
        mybytes = wav.readframes(chunk_size)
        if not mybytes or len(mybytes) == 0:
            break 
        await asyncio.sleep(delay)
        yield mybytes

async def get_mic_audio_bytes(chunk_size: int = 16000) -> bytes:
    stream = pyaudio.PyAudio().open(format=pyaudio.paInt16, channels=1, rate=16000, input=True, frames_per_buffer=chunk_size)
    while True:
        mybytes = stream.read(chunk_size)
        if not mybytes or len(mybytes) == 0:
            break 
        yield mybytes


async def main(source, chunk_size, server_address, delay, printfunc) -> None:
    print("in main")
    log.info(">>>>> Calling gRPC server at: "+server_address)
    async with grpc.aio.insecure_channel(server_address) as channel:
        stub = transcription_pb2_grpc.AudioTranscriptionStub(channel)
        await audio_transcription(stub, get_mic_audio_bytes(chunk_size) if source.lower() == "mic" else get_wav_audio_bytes(source, chunk_size, delay), printfunc)    


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Simple audio-sending gRPC client for audio transcription")
    parser.add_argument("source", help="Either 'mic' for using default microphone, or path of a WAV file (PCM s16le) to send to the service, in chunks")
    parser.add_argument("-c", "--chunk-size", dest="chunk_size", help="Size of each chunk (in samples) to send to the server", type=int, default=16000)
    parser.add_argument("-d", "--delay", dest="delay", help="Delay in seconds (but can pass float for ms) between each chunk send", type=float, default=0)
    parser.add_argument("-s", "--server-address", dest="server_address", help="Server address (host:port) of gRPC transcription server", default="localhost:50051")
    parser.add_argument("-v", "--verbose", dest="verbose", help="Verbose/debug logging mode", type=bool, default=False)
    args = parser.parse_args()
    log.basicConfig(level=log.DEBUG if args.verbose else log.INFO, format='%(asctime)s %(message)s')
    asyncio.get_event_loop().run_until_complete(main(args.source, args.chunk_size, args.server_address, args.delay, print))