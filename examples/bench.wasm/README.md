# bench.wasm

Benchmark the performance of whisper.cpp in the browser using WebAssembly

Link: https://whisper.ggerganov.com/bench/

Terminal version: [examples/bench](/examples/bench)

## Build instructions

```bash
# build using Emscripten (v3.1.2)
git clone https://github.com/ggerganov/whisper.cpp
cd whisper.cpp
mkdir build-em && cd build-em
emcmake cmake ..
make -j

# copy the produced page to your HTTP path
cp bin/bench.wasm/*       /path/to/html/
cp bin/libbench.worker.js /path/to/html/
```
