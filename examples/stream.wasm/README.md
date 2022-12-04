# stream.wasm

Real-time transcription in the browser using WebAssembly

Online demo: https://whisper.ggerganov.com/stream/

## Build instructions

```bash
# build using Emscripten (v3.1.2)
git clone https://github.com/ggerganov/whisper.cpp
cd whisper.cpp
mkdir build-em && cd build-em
emcmake cmake ..
make -j

# copy the produced page to your HTTP path
cp bin/stream.wasm/*       /path/to/html/
cp bin/libstream.worker.js /path/to/html/
```
