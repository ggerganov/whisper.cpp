# whisper.cpp

C/C++ port of [OpenAI's Whisper](https://github.com/openai/whisper) speech-to-text model

- Plain C/C++ implementation without dependencies
- ARM_NEON and AVX intrinsics support
- F16 support

## Usage

To build the main program, run `make`. You can then transribe a `.wav` file like this:

```bash
$ ./main -f input.wav
```

Before running the program, make sure to download one of the ggml Whisper models. For example:

```bash
bash ./download-ggml-model.sh base.en
```

---

For a quick demo, simply run `make base.en`:

```bash
$ make base.en

Downloading base.en (142 MB just once)
mkdir -p models
models/ggml-base.en.bin      100%[=================================>] 141.11M  7.50MB/s    in 19s

===============================================
Running base.en on all samples in ./samples ...
===============================================

----------------------------------------------
[+] Running base.en on samples/jfk.wav ... (run 'ffplay samples/jfk.wav' to listen)
----------------------------------------------

whisper_model_load: loading model from 'models/ggml-base.en.bin'
whisper_model_load: n_vocab       = 51864
whisper_model_load: n_audio_ctx   = 1500
whisper_model_load: n_audio_state = 512
whisper_model_load: n_audio_head  = 8
whisper_model_load: n_audio_layer = 6
whisper_model_load: n_text_ctx    = 448
whisper_model_load: n_text_state  = 512
whisper_model_load: n_text_head   = 8
whisper_model_load: n_text_layer  = 6
whisper_model_load: n_mels        = 80
whisper_model_load: f16           = 1
whisper_model_load: type          = 2
whisper_model_load: mem_required  = 782.00 MB
whisper_model_load: adding 1607 extra tokens
whisper_model_load: ggml ctx size = 186.26 MB
whisper_model_load: memory size =    45.66 MB
whisper_model_load: model size  =   140.54 MB
log_mel_spectrogram: n_sample = 176000, n_len = 1100
log_mel_spectrogram: recording length: 11.000000 s

 And so my fellow Americans ask not what your country can do for you. Ask what you can do for your country.

main:     load time =    60.62 ms
main:      mel time =    38.69 ms
main:   sample time =     2.36 ms
main:   encode time =   875.63 ms / 145.94 ms per layer
main:   decode time =   103.17 ms
main:    total time =  1081.13 ms

```

The command downloads the `base.en` model converted to custom `ggml` format and runs the inference on all `.wav` samples in the folder `samples`.

If you want some extra audio samples to play with, simply run:

```
make samples
```

This will download a few more audio files from Wikipedia and convert them to 16-bit WAV format via `ffmpeg`.

You can download and run the other `.en` models as follows:

```
make tiny.en
make base.en
make small.en
make medium.en
```

For detailed usage instructions, run: `./main -h`

Note that `whisper.cpp` runs only with 16-bit WAV files, so make sure to convert your input before running the tool.
For example, you can use `ffmpeg` like this:

```bash
ffmpeg -i input.mp3 -ar 16000 -ac 1 -c:a pcm_s16le output.wav
```

## Limitations

- Only `.en` models are supported
- Very basic greedy sampling scheme - always pick up the top token
- No timestamps
- English only
- Inference only
- Runs on the CPU
- Only mono-channel 16-bit WAV is supported

## Memory usage

| Model | Disk | Mem |
| ---   | --- | --- |
| tiny.en | 75 MB | ~600 MB |
| base.en | 142 MB | ~800 MB |
| small.en | 466 MB | ~1.6 GB |
| medium.en | 1.5 GB | ~3.5 GB |

## ggml format

The original models are converted to a custom binary format. This allows to pack everything needed into a single file:

- model parameters
- mel filters
- vocabulary
- weights

You can download the converted models using the [download-ggml-model.sh](download-ggml-model.sh) script.

For more details, see the conversion script [convert-pt-to-ggml.py](convert-pt-to-ggml.py)
