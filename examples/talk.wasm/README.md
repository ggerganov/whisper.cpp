# talk.wasm

Talk with an Artificial Intelligence entity in your browser:

https://user-images.githubusercontent.com/1991296/202914175-115793b1-d32e-4aaa-a45b-59e313707ff6.mp4

Online demo: https://talk.ggerganov.com

## How it works?

This demo leverages 2 modern neural network models to create a high-quality voice chat directly in your browser:

- [OpenAI's Whisper](https://github.com/openai/whisper) speech recognition model is used to process your voice and understand what you are saying
- Upon receiving some voice input, the AI generates a text response using [OpenAI's GPT-2](https://github.com/openai/gpt-2) language model
- The AI then vocalizes the response using the browser's [Web Speech API](https://developer.mozilla.org/en-US/docs/Web/API/Web_Speech_API)

The web page does the processing locally on your machine. However, in order to run the models, it first needs to
download the model data which is about ~350 MB. The model data is then cached in your browser's cache and can be reused
in future visits without downloading it again.

The processing of these heavy neural network models in the browser is possible by implementing them efficiently in C/C++
and using WebAssembly SIMD capabilities for extra performance. For more detailed information, checkout the
[current repository](https://github.com/ggerganov/whisper.cpp).

## Requirements

In order to run this demo efficiently, you need to have the following:

- Latest Chrome or Firefox browser (Safari is not supported)
- Run this on a desktop or laptop with modern CPU (a mobile phone will likely not be good enough)
- Speak phrases that are no longer than 10 seconds - this is the audio context of the AI
- The web-page uses about 1.4GB of RAM

## Feedback

If you have any comments or ideas for improvement, please drop a comment in the following discussion:

https://github.com/ggerganov/whisper.cpp/discussions/167
