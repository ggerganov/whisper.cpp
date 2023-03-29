# talk

Talk with an Artificial Intelligence in your terminal

[Demo Talk](https://user-images.githubusercontent.com/1991296/206805012-48e71cc2-588d-4745-8798-c1c70ea3b40d.mp4)

Web version: [examples/talk.wasm](/examples/talk.wasm)

## Building

The `talk` tool depends on SDL2 library to capture audio from the microphone. You can build it like this:

```bash
# Install SDL2 on Linux
sudo apt-get install libsdl2-dev

# Install SDL2 on Mac OS
brew install sdl2

# Build the "talk" executable
make talk

# Run it
./talk -p Santa
```

## GPT-2

To run this, you will need a ggml GPT-2 model: [instructions](https://github.com/ggerganov/ggml/tree/master/examples/gpt-2#downloading-and-converting-the-original-models)

Alternatively, you can simply download the smallest ggml GPT-2 117M model (240 MB) like this:

```
wget --quiet --show-progress -O models/ggml-gpt-2-117M.bin https://huggingface.co/ggerganov/ggml/raw/main/ggml-model-gpt-2-117M.bin
```

## TTS

For best experience, this example needs a TTS tool to convert the generated text responses to voice.
You can use any TTS engine that you would like - simply edit the [speak.sh](speak.sh) script to your needs.
By default, it is configured to use `espeak`, but you can use whatever you wish.
