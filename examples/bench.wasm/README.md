# bench.wasm

Benchmark the performance of whisper.cpp in the browser using WebAssembly

Link: https://ggerganov.github.io/whisper.cpp/bench.wasm

Terminal version: [examples/bench](/examples/bench)

## Build instructions

```bash
# build using Emscripten (v3.1.2)
git clone https://github.com/ggerganov/whisper.cpp
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
http://localhost:8000/bench.wasm

To run the example in a different server, you need to copy the following files
to the server's HTTP path:
```
# copy the produced page to your HTTP path
cp bin/bench.wasm/*       /path/to/html/
cp bin/libbench.worker.js /path/to/html/
```
