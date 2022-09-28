# whisper.cpp

C/C++ port of [OpenAI's Whisper](https://github.com/openai/whisper) speech-to-text model

- Plain C/C++ implementation without dependencies
- ARM_NEON and AVX intrinsics support
- Mixed F16 / F32 support
- Low memory usage (Flash Attention + Flash Forward)

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

gcc -pthread -O3 -mavx -mavx2 -mfma -mf16c -c ggml.c
g++ -pthread -O3 -std=c++11 -c main.cpp
g++ -o main ggml.o main.o
./main -h

usage: ./main [options]

options:
  -h,       --help           show this help message and exit
  -s SEED,  --seed SEED      RNG seed (default: -1)
  -t N,     --threads N      number of threads to use during computation (default: 4)
  -T N,     --tokens N       maximum number of tokens to generate per iteration (default: 64)
  -v,       --verbose        verbose output
            --translate      translate from source language to english
  -ps,      --print_special  print special tokens
  -l LANG,  --language LANG  spoken language (default: en)
  -m FNAME, --model FNAME    model path (default: models/ggml-base.en.bin)
  -f FNAME, --file FNAME     input WAV file path (default: samples/jfk.wav)

bash ./download-ggml-model.sh base.en
Downloading ggml model base.en ...
models/ggml-base.en.bin         100%[=====================================>] 141.11M  7.84MB/s    in 18s
Done! Model 'base.en' saved in 'models/ggml-base.en.bin'
You can now use it like this:

  $ ./main -m models/ggml-base.en.bin -f samples/jfk.wav


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
whisper_model_load: mem_required  = 611.00 MB
whisper_model_load: adding 1607 extra tokens
whisper_model_load: ggml ctx size = 163.43 MB
whisper_model_load: memory size =    22.83 MB
whisper_model_load: model size  =   140.54 MB
log_mel_spectrogram: n_sample = 176000, n_len = 1100
log_mel_spectrogram: recording length: 11.000000 s

main: processing 176000 samples (11.0 sec), 4 threads, lang = english, task = transcribe ...

 And so my fellow Americans ask not what your country can do for you. Ask what you can do for your country.

main:     load time =    71.89 ms
main:      mel time =    36.95 ms
main:   sample time =     2.10 ms
main:   encode time =   700.94 ms / 116.82 ms per layer
main:   decode time =    86.14 ms
main:    total time =   898.72 ms
```

The command downloads the `base.en` model converted to custom `ggml` format and runs the inference on all `.wav` samples in the folder `samples`.

If you want some extra audio samples to play with, simply run:

```
make samples
```

This will download a few more audio files from Wikipedia and convert them to 16-bit WAV format via `ffmpeg`.

You can download and run the other models as follows:

```
make tiny.en
make tiny
make base.en
make base
make small.en
make small
make medium.en
make medium
make large
```

For detailed usage instructions, run: `./main -h`

Note that `whisper.cpp` runs only with 16-bit WAV files, so make sure to convert your input before running the tool.
For example, you can use `ffmpeg` like this:

```bash
ffmpeg -i input.mp3 -ar 16000 -ac 1 -c:a pcm_s16le output.wav
```

## Limitations

- Very basic greedy sampling scheme - always pick up the top token
- No timestamps
- Inference only
- Runs on the CPU
- Only mono-channel 16-bit WAV is supported

## Memory usage

| Model | Disk | Mem |
| ---   | --- | --- |
| tiny | 75 MB | ~460 MB |
| base | 142 MB | ~620 MB |
| small | 466 MB | ~1.3 GB |
| medium | 1.5 GB | ~2.8 GB |
| large | 2.9 GB | ~4.9 GB |

## ggml format

The original models are converted to a custom binary format. This allows to pack everything needed into a single file:

- model parameters
- mel filters
- vocabulary
- weights

You can download the converted models using the [download-ggml-model.sh](download-ggml-model.sh) script.

For more details, see the conversion script [convert-pt-to-ggml.py](convert-pt-to-ggml.py)
