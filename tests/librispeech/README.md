# whisper.cpp/tests/librispeech

[LibriSpeech](https://www.openslr.org/12) is a standard dataset for
training and evaluating automatic speech recognition systems.

This directory contains a set of tools to evaluate the recognition
performance of whisper.cpp on LibriSpeech corpus.

## Quick Start

1. (Pre-requirement) Compile `whisper-cli` and prepare the Whisper
   model in `ggml` format.

   - Consult [whisper.cpp/README.md](../../README.md).

2. Download the audio files from LibriSpeech project.

   ```
   $ make get-audio
   ```

3. Set up the environment to compute WER score.

   ```
   $ make setup-venv
   ```

4. Run the benchmark test.

   ```
   $ make
   ```

## How-to guides

### How to change the inferece parameters

Create `eval.conf` and override variables.

```
WHISPER_MODEL = large-v3-turbo
WHISPER_FLAGS = --no-prints --threads 8 --language en --output-txt
```

Check out `eval.mk` for more details.
