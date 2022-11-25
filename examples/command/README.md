# command

This is a basic Voice Assistant example that accepts voice commands from the microphone.
More info is available in [issue #171](https://github.com/ggerganov/whisper.cpp/issues/171).

```java
# Run with default arguments and small model
./command -m ./models/ggml-small.en.bin -t 8

# On Raspberry Pi, use tiny or base models + "-ac 768" for better performance
./bin/command -m ../models/ggml-tiny.en.bin -ac 768
```

## Building

The `command` tool depends on SDL2 library to capture audio from the microphone. You can build it like this:

```bash
# Install SDL2 on Linux
sudo apt-get install libsdl2-dev

# Install SDL2 on Mac OS
brew install sdl2

make command
```
