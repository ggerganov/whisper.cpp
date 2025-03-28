# command.wasm

This is a basic Voice Assistant example that accepts voice commands from the microphone.
It runs in fully in the browser via WebAseembly.

Online demo: https://whisper.ggerganov.com/command/

Terminal version: [examples/command](/examples/command)

## Build instructions

```bash
# build using Emscripten (v3.1.2)
git clone https://github.com/ggerganov/whisper.cpp
cd whisper.cpp
mkdir build-em && cd build-em
emcmake cmake ..
make -j libcommand
```
The example can then be started by running a local HTTP server:
```console
python3 examples/server.py
```
And then opening a browser to the following URL:
http://localhost:8000/command.wasm/

To run the example in a different server, you need to copy the following files
to the server's HTTP path:
```
cp bin/command.wasm/*       /path/to/html/
cp bin/libcommand.worker.js /path/to/html/
```
