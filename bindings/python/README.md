# Python bindings for Whisper

This is a guide on Python bindings for whisper.cpp. It has been tested on:

  * Darwin (OS X) 12.6 on x64_64
  * Debian Linux on arm64
  * Ubuntu x86_64


## Usage
It can be used like this:

  * move the compiled 'libwhisper.so' to the same directory, or add it to the path.
  * rebuild the low-level wrapper is something breaks on changes to whisper-cpp (see below).

```python
from scipy.io import wavfile
from whisper_cpp import WhisperCpp

# prepare audio data
samplerate, audio = wavfile.read("samples/jfk.wav")
audio = audio.astype("float32") / 32768.0

# run the inference
model = WhisperCpp(model="./models/ggml-medium.en.bin")
transcription = model.transcribe(audio)

print(transcription)
```

## Rebuilding

The "low level" bindings are autogenerate and can be simply regenerated as follows:

```bash
# from the root whisper.pp directory
> ctypesgen whisper.h -l whisper -o whisper_cpp_wrapper.py
```

The interface file will probable need to be rewritten manually on big changes to whisper.cpp, but is relatively easy (compared to manual wrapping!).