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
- The `--session` argument specifies a file to save/load the LLaMA model state. When provided, the LLaMA model's state will be saved to the given file after each interaction. This allows for a more coherent and continuous conversation with the LLaMA assistant, as it maintains context from previous interactions. If the file does not exist, it will be created. If the file exists, the model state will be loaded from it, allowing you to resume a previous session.

## TTS

For best experience, this example needs a TTS tool to convert the generated text responses to voice.
You can use any TTS engine that you would like - simply edit the [speak.sh](speak.sh) script to your needs.
By default, it is configured to use MacOS's `say`, but you can use whatever you wish.

## Discussion

If you have any feedback, please let "us" know in the following discussion: https://github.com/ggerganov/whisper.cpp/discussions/672?converting=1
