# stream.wasm

Real-time transcription in the browser using WebAssembly

Online demo: https://whisper.ggerganov.com/stream/

## Build instructions

```bash
# build using Emscripten (v3.1.2)
git clone --recurse-submodules https://github.com/ggerganov/whisper.cpp.git
cd whisper.cpp
mkdir build-em && cd build-em
emcmake cmake ..
make -j
```
The example can then be started by running a local HTTP server:
```console
python3 examples/server.py
```
And then opening a browser to the following URL:
http://localhost:8000/stream.wasm

To run the example in a different server, you need to copy the following files
to the server's HTTP path:
```
# copy the produced page to your HTTP path
cp bin/stream.wasm/*       /path/to/html/
cp bin/libstream.worker.js /path/to/html/
```
