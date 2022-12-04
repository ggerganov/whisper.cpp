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
make -j

# copy the produced page to your HTTP path
cp bin/command.wasm/*       /path/to/html/
cp bin/libcommand.worker.js /path/to/html/
```
