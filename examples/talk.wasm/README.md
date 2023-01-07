# talk.wasm

Talk with an Artificial Intelligence in your browser:

[https://user-images.githubusercontent.com/1991296/203411580-fedb4839-05e4-4474-8364-aaf1e9a9b615.mp4](https://user-images.githubusercontent.com/1991296/203845553-f7b44e13-9a15-4fc8-b518-ae8f4c6770fe.mp4)

Online demo: https://whisper.ggerganov.com/talk/

Terminal version: [examples/talk](/examples/talk)

## How it works?

This demo leverages 2 modern neural network models to create a high-quality voice chat directly in your browser:

- [OpenAI's Whisper](https://github.com/openai/whisper) speech recognition model is used to process your voice and understand what you are saying
- Upon receiving some voice input, the AI generates a text response using [OpenAI's GPT-2](https://github.com/openai/gpt-2) language model
- The AI then vocalizes the response using the browser's [Web Speech API](https://developer.mozilla.org/en-US/docs/Web/API/Web_Speech_API)

The web page does the processing locally on your machine. The processing of these heavy neural network models in the
browser is possible by implementing them efficiently in C/C++ and using the browser's WebAssembly SIMD capabilities for
extra performance:

- The Whisper C++ implementation is here: [whisper.h](/whisper.h) / [whisper.cpp](/whisper.cpp)
- The GPT-2 C++ implementation is here: [gpt-2.h](gpt-2.h) / [gpt-2.cpp](gpt-2.cpp)
- Both models use a custom tensor library implemented in C: [ggml.h](/ggml.h) / [ggml.c](/ggml.c)
- The HTML/JS layer is here: [index-tmpl.html](index-tmpl.html)
- The Emscripten bridge between C/C++ and JS is here: [emscripten.cpp](emscripten.cpp)

In order to run the models, the web page first needs to download the model data which is about ~350 MB. The model data
is then cached in your browser's cache and can be reused in future visits without downloading it again.

## Requirements

In order to run this demo efficiently, you need to have the following:

- Latest Chrome or Firefox browser (Safari is not supported)
- Run this on a desktop or laptop with modern CPU (a mobile phone will likely not be good enough)
- Speak phrases that are no longer than 10 seconds - this is the audio context of the AI
- The web-page uses about 1.8GB of RAM

Notice that this demo is using the smallest GPT-2 model, so the generated text responses are not always very good.
Also, the prompting strategy can likely be improved to achieve better results.

The demo is quite computationally heavy, so you need a fast CPU. It's not usual to run these transformer models in a
browser. Typically, they run on powerful GPUs.

Currently, mobile browsers do not support the Fixed-width SIMD WebAssembly capability, so you cannot run this demo
on a phone or a tablet. Hopefully, in the near future this will become supported.

## Todo

- Better UI (contributions are welcome)
- Better GPT-2 prompting

## Build instructions

```bash
# build using Emscripten (v3.1.2)
git clone https://github.com/ggerganov/whisper.cpp
cd whisper.cpp
mkdir build-em && cd build-em
emcmake cmake ..
make -j

# copy the produced page to your HTTP path
cp bin/talk.wasm/*       /path/to/html/
cp bin/libtalk.worker.js /path/to/html/
```

## Feedback

If you have any comments or ideas for improvement, please drop a comment in the following discussion:

https://github.com/ggerganov/whisper.cpp/discussions/167
