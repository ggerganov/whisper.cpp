# whisper.cpp

[![Actions Status](https://github.com/ggerganov/whisper.cpp/workflows/CI/badge.svg)](https://github.com/ggerganov/whisper.cpp/actions)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](https://opensource.org/licenses/MIT)

High-performance inference of [OpenAI's Whisper](https://github.com/openai/whisper) automatic speech recognition (ASR) model:

- Plain C/C++ implementation without dependencies
- Apple silicon first-class citizen - optimized via Arm Neon and Accelerate framework
- AVX intrinsics support for x86 architectures
- Mixed F16 / F32 precision
- Low memory usage (Flash Attention + Flash Forward)
- Zero memory allocations at runtime
- Runs on the CPU
- [C-style API](https://github.com/ggerganov/whisper.cpp/blob/master/whisper.h)

Supported platforms:

- [x] Mac OS (Intel and Arm)
- [x] [iOS](examples/whisper.objc)
- [x] Linux
- [x] [WebAssembly](examples/whisper.wasm)
- [x] [Windows (MSVC and MinGW)](https://github.com/ggerganov/whisper.cpp/issues/5)
- [x] [Raspberry Pi](https://github.com/ggerganov/whisper.cpp/issues/7)
- [x] [Android](https://github.com/ggerganov/whisper.cpp/issues/30)

The entire implementation of the model is contained in 2 source files:

- [ggml.h](ggml.h) / [ggml.c](ggml.c)
- [whisper.h](whisper.h) / [whisper.cpp](whisper.cpp)

Having such a lightweight implementation of the model allows to easily integrate it in different platforms and applications.
As an example, here is a video of running the model on an iPhone 13 device - fully offline, on-device:

https://user-images.githubusercontent.com/1991296/197385372-962a6dea-bca1-4d50-bf96-1d8c27b98c81.mp4

## Quick start

First, download one of the Whisper models converted in [ggml format](models). For example:

```bash
bash ./models/download-ggml-model.sh base.en
```

Now build the [main](examples/main) example and transcribe an audio file like this:

```bash
# build the main example
make

# transcribe an audio file
./main -f input.wav
```

---

For a quick demo, simply run `make base.en`:

```java
$ make base.en

cc  -I.              -O3 -std=c11   -pthread -DGGML_USE_ACCELERATE   -c ggml.c
c++ -I. -I./examples -O3 -std=c++11 -pthread -c whisper.cpp
c++ -I. -I./examples -O3 -std=c++11 -pthread examples/main/main.cpp whisper.o ggml.o -o main  -framework Accelerate
./main -h

usage: ./main [options] file0.wav file1.wav ...

options:
  -h,       --help           show this help message and exit
  -s SEED,  --seed SEED      RNG seed (default: -1)
  -t N,     --threads N      number of threads to use during computation (default: 4)
  -ot N,    --offset-t N     time offset in milliseconds (default: 0)
  -on N,    --offset-n N     segment index offset (default: 0)
  -v,       --verbose        verbose output
            --translate      translate from source language to english
  -otxt,    --output-txt     output result in a text file
  -ovtt,    --output-vtt     output result in a vtt file
  -osrt,    --output-srt     output result in a srt file
  -ps,      --print_special  print special tokens
  -pc,      --print_colors   print colors
  -nt,      --no_timestamps  do not print timestamps
  -l LANG,  --language LANG  spoken language (default: en)
  -m FNAME, --model FNAME    model path (default: models/ggml-base.en.bin)
  -f FNAME, --file FNAME     input WAV file path

bash ./models/download-ggml-model.sh base.en
Downloading ggml model base.en ...
ggml-base.en.bin               100%[========================>] 141.11M  6.34MB/s    in 24s     
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
whisper_model_load: mem_required  = 505.00 MB
whisper_model_load: adding 1607 extra tokens
whisper_model_load: ggml ctx size = 163.43 MB
whisper_model_load: memory size =    22.83 MB
whisper_model_load: model size  =   140.54 MB

main: processing 'samples/jfk.wav' (176000 samples, 11.0 sec), 4 threads, lang = en, task = transcribe, timestamps = 1 ...

[00:00.000 --> 00:11.000]   And so my fellow Americans, ask not what your country can do for you, ask what you can do for your country.


whisper_print_timings:     load time =    87.21 ms
whisper_print_timings:      mel time =    24.26 ms
whisper_print_timings:   sample time =     3.87 ms
whisper_print_timings:   encode time =   323.67 ms / 53.94 ms per layer
whisper_print_timings:   decode time =    83.25 ms / 13.87 ms per layer
whisper_print_timings:    total time =   522.66 ms
```

The command downloads the `base.en` model converted to custom `ggml` format and runs the inference on all `.wav` samples in the folder `samples`.

For detailed usage instructions, run: `./main -h`

Note that the [main](examples/main) example currently runs only with 16-bit WAV files, so make sure to convert your input before running the tool.
For example, you can use `ffmpeg` like this:

```java
ffmpeg -i input.mp3 -ar 16000 -ac 1 -c:a pcm_s16le output.wav
```

## More audio samples

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

## Another example

Here is another example of transcribing a [3:24 min speech](https://upload.wikimedia.org/wikipedia/commons/1/1f/George_W_Bush_Columbia_FINAL.ogg)
in about half a minute on a MacBook M1 Pro, using `medium.en` model:

<details>
  <summary>Expand to see the result</summary>
  
```java
$ ./main -m models/ggml-medium.en.bin -f samples/gb1.wav -t 8

whisper_model_load: loading model from 'models/ggml-medium.en.bin'
whisper_model_load: n_vocab       = 51864
whisper_model_load: n_audio_ctx   = 1500
whisper_model_load: n_audio_state = 1024
whisper_model_load: n_audio_head  = 16
whisper_model_load: n_audio_layer = 24
whisper_model_load: n_text_ctx    = 448
whisper_model_load: n_text_state  = 1024
whisper_model_load: n_text_head   = 16
whisper_model_load: n_text_layer  = 24
whisper_model_load: n_mels        = 80
whisper_model_load: f16           = 1
whisper_model_load: type          = 4
whisper_model_load: mem_required  = 2610.00 MB
whisper_model_load: adding 1607 extra tokens
whisper_model_load: ggml ctx size = 1644.97 MB
whisper_model_load: memory size =   182.62 MB
whisper_model_load: model size  =  1462.12 MB

main: processing 'samples/gb1.wav' (3179750 samples, 198.7 sec), 8 threads, lang = en, task = transcribe, timestamps = 1 ...

[00:00.000 --> 00:08.000]   My fellow Americans, this day has brought terrible news and great sadness to our country.
[00:08.000 --> 00:17.000]   At nine o'clock this morning, Mission Control in Houston lost contact with our Space Shuttle Columbia.
[00:17.000 --> 00:23.000]   A short time later, debris was seen falling from the skies above Texas.
[00:23.000 --> 00:29.000]   The Columbia's lost. There are no survivors.
[00:29.000 --> 00:32.000]   On board was a crew of seven.
[00:32.000 --> 00:39.000]   Colonel Rick Husband, Lieutenant Colonel Michael Anderson, Commander Laurel Clark,
[00:39.000 --> 00:48.000]   Captain David Brown, Commander William McCool, Dr. Kultna Shavla, and Ilan Ramon,
[00:48.000 --> 00:52.000]   a colonel in the Israeli Air Force.
[00:52.000 --> 00:58.000]   These men and women assumed great risk in the service to all humanity.
[00:58.000 --> 01:03.000]   In an age when space flight has come to seem almost routine,
[01:03.000 --> 01:07.000]   it is easy to overlook the dangers of travel by rocket
[01:07.000 --> 01:12.000]   and the difficulties of navigating the fierce outer atmosphere of the Earth.
[01:12.000 --> 01:18.000]   These astronauts knew the dangers, and they faced them willingly,
[01:18.000 --> 01:23.000]   knowing they had a high and noble purpose in life.
[01:23.000 --> 01:31.000]   Because of their courage and daring and idealism, we will miss them all the more.
[01:31.000 --> 01:36.000]   All Americans today are thinking as well of the families of these men and women
[01:36.000 --> 01:40.000]   who have been given this sudden shock and grief.
[01:40.000 --> 01:45.000]   You're not alone. Our entire nation grieves with you,
[01:45.000 --> 01:52.000]   and those you love will always have the respect and gratitude of this country.
[01:52.000 --> 01:56.000]   The cause in which they died will continue.
[01:56.000 --> 02:04.000]   Mankind is led into the darkness beyond our world by the inspiration of discovery
[02:04.000 --> 02:11.000]   and the longing to understand. Our journey into space will go on.
[02:11.000 --> 02:16.000]   In the skies today, we saw destruction and tragedy.
[02:16.000 --> 02:22.000]   Yet farther than we can see, there is comfort and hope.
[02:22.000 --> 02:29.000]   In the words of the prophet Isaiah, "Lift your eyes and look to the heavens
[02:29.000 --> 02:35.000]   who created all these. He who brings out the starry hosts one by one
[02:35.000 --> 02:39.000]   and calls them each by name."
[02:39.000 --> 02:46.000]   Because of His great power and mighty strength, not one of them is missing.
[02:46.000 --> 02:55.000]   The same Creator who names the stars also knows the names of the seven souls we mourn today.
[02:55.000 --> 03:01.000]   The crew of the shuttle Columbia did not return safely to earth,
[03:01.000 --> 03:05.000]   yet we can pray that all are safely home.
[03:05.000 --> 03:13.000]   May God bless the grieving families, and may God continue to bless America.
[03:13.000 --> 03:41.000]   Audio


whisper_print_timings:     load time =   575.92 ms
whisper_print_timings:      mel time =   230.60 ms
whisper_print_timings:   sample time =    73.19 ms
whisper_print_timings:   encode time = 19552.61 ms / 814.69 ms per layer
whisper_print_timings:   decode time = 13249.96 ms / 552.08 ms per layer
whisper_print_timings:    total time = 33686.27 ms
```
</details>

## Real-time audio input example

This is a naive example of performing real-time inference on audio from your microphone.
The [stream](examples/stream) tool samples the audio every half a second and runs the transcription continously.
More info is available in [issue #10](https://github.com/ggerganov/whisper.cpp/issues/10).

```java
./stream -m ./models/ggml-base.en.bin -t 8 --step 500 --length 5000
```

https://user-images.githubusercontent.com/1991296/194935793-76afede7-cfa8-48d8-a80f-28ba83be7d09.mp4

## Confidence color-coding

Adding the `--print-colors` argument will print the transcribed text using an experimental color coding strategy
to highlight words with high or low confidence:

<img width="965" alt="image" src="https://user-images.githubusercontent.com/1991296/197356445-311c8643-9397-4e5e-b46e-0b4b4daa2530.png">

## Implementation details

- The core tensor operations are implemented in C ([ggml.h](ggml.h) / [ggml.c](ggml.c))
- The high-level C-style API is implemented in C++ ([whisper.h](whisper.h) / [whisper.cpp](whisper.cpp))
- Sample usage is demonstrated in [main.cpp](examples/main)
- Sample real-time audio transcription from the microphone is demonstrated in [stream.cpp](examples/stream)
- Various other examples are available in the [examples](examples) folder

The tensor operators are optimized heavily for Apple silicon CPUs. Depending on the computation size, Arm Neon SIMD
instrisics or CBLAS Accelerate framework routines are used. The latter are especially effective for bigger sizes since
the Accelerate framework utilizes the special-purpose AMX coprocessor available in modern Apple products.

## Limitations

- Inference only
- No GPU support
- Very basic greedy sampling scheme - always pick up the token with highest probability.
  This should be similar to the [GreedyDecoder](https://github.com/openai/whisper/blob/main/whisper/decoding.py#L249-L274)
  from the original python implementation, so in order to make a fair comparison between the 2 implementations, make sure
  to run the python code with the following parameters:

  ```
  whisper --best_of None --beam_size None ...
  ```

  In the future, `whisper.cpp` will support more sampling strategies.

## Memory usage

| Model  | Disk   | Mem     |
| ---    | ---    | ---     |
| tiny   |  75 MB | ~280 MB |
| base   | 142 MB | ~430 MB |
| small  | 466 MB | ~1.0 GB |
| medium | 1.5 GB | ~2.6 GB |
| large  | 2.9 GB | ~4.7 GB |

## Benchmarks

In order to have an objective comparison of the performance of the inference across different system configurations,
use the [bench](examples/bench) tool. The tool simply runs the Encoder part of the model and prints how much time it
took to execute it. The results are summarized in the following Github issue:

[Benchmark results](https://github.com/ggerganov/whisper.cpp/issues/89)

## ggml format

The original models are converted to a custom binary format. This allows to pack everything needed into a single file:

- model parameters
- mel filters
- vocabulary
- weights

You can download the converted models using the [models/download-ggml-model.sh](models/download-ggml-model.sh) script or from here:

https://ggml.ggerganov.com

For more details, see the conversion script [models/convert-pt-to-ggml.py](models/convert-pt-to-ggml.py) or the README in [models](models).

## Bindings

- [X] Rust: [tazz4843/whisper-rs](https://github.com/tazz4843/whisper-rs)
- [ ] Python:
- [ ] Java:

## Examples

There are various examples of using the library for different projects in the [examples](examples) folder. Check them out!
