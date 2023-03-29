# talk-llama

Talk with an LLaMA AI in your terminal

[Demo Talk](https://user-images.githubusercontent.com/1991296/228024237-848f998c-c334-46a6-bef8-3271590da83b.mp4)

## Building

The `talk-llama` tool depends on SDL2 library to capture audio from the microphone. You can build it like this:

```bash
# Install SDL2 on Linux
sudo apt-get install libsdl2-dev

# Install SDL2 on Mac OS
brew install sdl2

# Build the "talk-llama" executable
make talk-llama

# Run it
./talk-llama -mw ./models/ggml-small.en.bin -ml ../llama.cpp/models/13B/ggml-model-q4_0.bin -p "Georgi" -t 8
```

- The `-mw` argument specifies the Whisper model that you would like to use. Recommended `base` or `small` for real-time experience
- The `-ml` argument specifies the LLaMA model that you would like to use. Read the instructions in https://github.com/ggerganov/llama.cpp for information about how to obtain a `ggml` compatible LLaMA model

## TTS

For best experience, this example needs a TTS tool to convert the generated text responses to voice.
You can use any TTS engine that you would like - simply edit the [speak.sh](speak.sh) script to your needs.
By default, it is configured to use MacOS's `say`, but you can use whatever you wish.

## Discussion

If you have any feedback, please let "us" know in the following discussion: https://github.com/ggerganov/whisper.cpp/discussions/672?converting=1
