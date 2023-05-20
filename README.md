# whisper.cpp

![whisper.cpp](https://user-images.githubusercontent.com/1991296/235238348-05d0f6a4-da44-4900-a1de-d0707e75b763.jpeg)

[![Actions Status](https://github.com/ggerganov/whisper.cpp/workflows/CI/badge.svg)](https://github.com/ggerganov/whisper.cpp/actions)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](https://opensource.org/licenses/MIT)
[![npm](https://img.shields.io/npm/v/whisper.cpp.svg)](https://www.npmjs.com/package/whisper.cpp/)

Beta: [v1.4.2](https://github.com/ggerganov/whisper.cpp/releases/tag/v1.4.2) / Stable: [v1.2.1](https://github.com/ggerganov/whisper.cpp/releases/tag/v1.2.1) / [Roadmap | F.A.Q.](https://github.com/ggerganov/whisper.cpp/discussions/126)

High-performance inference of [OpenAI's Whisper](https://github.com/openai/whisper) automatic speech recognition (ASR) model:

- Plain C/C++ implementation without dependencies
- Apple silicon first-class citizen - optimized via ARM NEON, Accelerate framework and [Core ML](https://github.com/ggerganov/whisper.cpp#core-ml-support)
- AVX intrinsics support for x86 architectures
- VSX intrinsics support for POWER architectures
- Mixed F16 / F32 precision
- [4-bit and 5-bit integer quantization support](https://github.com/ggerganov/whisper.cpp#quantization)
- Low memory usage (Flash Attention)
- Zero memory allocations at runtime
- Runs on the CPU
- [Partial GPU support for NVIDIA via cuBLAS](https://github.com/ggerganov/whisper.cpp#nvidia-gpu-support-via-cublas)
- [Partial OpenCL GPU support via CLBlast](https://github.com/ggerganov/whisper.cpp#opencl-gpu-support-via-clblast)
- [C-style API](https://github.com/ggerganov/whisper.cpp/blob/master/whisper.h)

Supported platforms:

- [x] Mac OS (Intel and Arm)
- [x] [iOS](examples/whisper.objc)
- [x] [Android](examples/whisper.android)
- [x] [Java](bindings/java/README.md)
- [x] Linux / [FreeBSD](https://github.com/ggerganov/whisper.cpp/issues/56#issuecomment-1350920264)
- [x] [WebAssembly](examples/whisper.wasm)
- [x] Windows ([MSVC](https://github.com/ggerganov/whisper.cpp/blob/master/.github/workflows/build.yml#L117-L144) and [MinGW](https://github.com/ggerganov/whisper.cpp/issues/168)]
- [x] [Raspberry Pi](https://github.com/ggerganov/whisper.cpp/discussions/166)

The entire implementation of the model is contained in 2 source files:

- Tensor operations: [ggml.h](ggml.h) / [ggml.c](ggml.c)
- Transformer inference: [whisper.h](whisper.h) / [whisper.cpp](whisper.cpp)

Having such a lightweight implementation of the model allows to easily integrate it in different platforms and applications.
As an example, here is a video of running the model on an iPhone 13 device - fully offline, on-device: [whisper.objc](examples/whisper.objc)

https://user-images.githubusercontent.com/1991296/197385372-962a6dea-bca1-4d50-bf96-1d8c27b98c81.mp4

You can also easily make your own offline voice assistant application: [command](examples/command)

https://user-images.githubusercontent.com/1991296/204038393-2f846eae-c255-4099-a76d-5735c25c49da.mp4

Or you can even run it straight in the browser: [talk.wasm](examples/talk.wasm)

## Implementation details

- The core tensor operations are implemented in C ([ggml.h](ggml.h) / [ggml.c](ggml.c))
- The transformer model and the high-level C-style API are implemented in C++ ([whisper.h](whisper.h) / [whisper.cpp](whisper.cpp))
- Sample usage is demonstrated in [main.cpp](examples/main)
- Sample real-time audio transcription from the microphone is demonstrated in [stream.cpp](examples/stream)
- Various other examples are available in the [examples](examples) folder

The tensor operators are optimized heavily for Apple silicon CPUs. Depending on the computation size, Arm Neon SIMD
instrisics or CBLAS Accelerate framework routines are used. The latter are especially effective for bigger sizes since
the Accelerate framework utilizes the special-purpose AMX coprocessor available in modern Apple products.

## Quick start

First clone the repository.

Then, download one of the Whisper models converted in [ggml format](models). For example:

```bash
bash ./models/download-ggml-model.sh base.en
```

If you wish to convert the Whisper models to ggml format yourself, instructions are in [models/README.md](models/README.md).

Now build the [main](examples/main) example and transcribe an audio file like this:

```bash
# build the main example
make

# transcribe an audio file
./main -f samples/jfk.wav
```

---

For a quick demo, simply run `make base.en`:

```java
$ make base.en

cc  -I.              -O3 -std=c11   -pthread -DGGML_USE_ACCELERATE   -c ggml.c -o ggml.o
c++ -I. -I./examples -O3 -std=c++11 -pthread -c whisper.cpp -o whisper.o
c++ -I. -I./examples -O3 -std=c++11 -pthread examples/main/main.cpp whisper.o ggml.o -o main  -framework Accelerate
./main -h

usage: ./main [options] file0.wav file1.wav ...

options:
  -h,        --help              [default] show this help message and exit
  -t N,      --threads N         [4      ] number of threads to use during computation
  -p N,      --processors N      [1      ] number of processors to use during computation
  -ot N,     --offset-t N        [0      ] time offset in milliseconds
  -on N,     --offset-n N        [0      ] segment index offset
  -d  N,     --duration N        [0      ] duration of audio to process in milliseconds
  -mc N,     --max-context N     [-1     ] maximum number of text context tokens to store
  -ml N,     --max-len N         [0      ] maximum segment length in characters
  -bo N,     --best-of N         [5      ] number of best candidates to keep
  -bs N,     --beam-size N       [-1     ] beam size for beam search
  -wt N,     --word-thold N      [0.01   ] word timestamp probability threshold
  -et N,     --entropy-thold N   [2.40   ] entropy threshold for decoder fail
  -lpt N,    --logprob-thold N   [-1.00  ] log probability threshold for decoder fail
  -su,       --speed-up          [false  ] speed up audio by x2 (reduced accuracy)
  -tr,       --translate         [false  ] translate from source language to english
  -di,       --diarize           [false  ] stereo audio diarization
  -nf,       --no-fallback       [false  ] do not use temperature fallback while decoding
  -otxt,     --output-txt        [false  ] output result in a text file
  -ovtt,     --output-vtt        [false  ] output result in a vtt file
  -osrt,     --output-srt        [false  ] output result in a srt file
  -owts,     --output-words      [false  ] output script for generating karaoke video
  -ocsv,     --output-csv        [false  ] output result in a CSV file
  -of FNAME, --output-file FNAME [       ] output file path (without file extension)
  -ps,       --print-special     [false  ] print special tokens
  -pc,       --print-colors      [false  ] print colors
  -pp,       --print-progress    [false  ] print progress
  -nt,       --no-timestamps     [true   ] do not print timestamps
  -l LANG,   --language LANG     [en     ] spoken language ('auto' for auto-detect)
             --prompt PROMPT     [       ] initial prompt
  -m FNAME,  --model FNAME       [models/ggml-base.en.bin] model path
  -f FNAME,  --file FNAME        [       ] input WAV file path


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

whisper_init_from_file: loading model from 'models/ggml-base.en.bin'
whisper_model_load: loading model
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
whisper_model_load: mem required  =  215.00 MB (+    6.00 MB per decoder)
whisper_model_load: kv self size  =    5.25 MB
whisper_model_load: kv cross size =   17.58 MB
whisper_model_load: adding 1607 extra tokens
whisper_model_load: model ctx     =  140.60 MB
whisper_model_load: model size    =  140.54 MB

system_info: n_threads = 4 / 10 | AVX = 0 | AVX2 = 0 | AVX512 = 0 | FMA = 0 | NEON = 1 | ARM_FMA = 1 | F16C = 0 | FP16_VA = 1 | WASM_SIMD = 0 | BLAS = 1 | SSE3 = 0 | VSX = 0 |

main: processing 'samples/jfk.wav' (176000 samples, 11.0 sec), 4 threads, 1 processors, lang = en, task = transcribe, timestamps = 1 ...


[00:00:00.000 --> 00:00:11.000]   And so my fellow Americans, ask not what your country can do for you, ask what you can do for your country.


whisper_print_timings:     fallbacks =   0 p /   0 h
whisper_print_timings:     load time =   113.81 ms
whisper_print_timings:      mel time =    15.40 ms
whisper_print_timings:   sample time =    11.58 ms /    27 runs (    0.43 ms per run)
whisper_print_timings:   encode time =   266.60 ms /     1 runs (  266.60 ms per run)
whisper_print_timings:   decode time =    66.11 ms /    27 runs (    2.45 ms per run)
whisper_print_timings:    total time =   476.31 ms
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
make large-v1
make large
```

## Memory usage

| Model  | Disk   | Mem     | SHA                                        |
| ---    | ---    | ---     | ---                                        |
| tiny   |  75 MB | ~125 MB | `bd577a113a864445d4c299885e0cb97d4ba92b5f` |
| base   | 142 MB | ~210 MB | `465707469ff3a37a2b9b8d8f89f2f99de7299dac` |
| small  | 466 MB | ~600 MB | `55356645c2b361a969dfd0ef2c5a50d530afd8d5` |
| medium | 1.5 GB | ~1.7 GB | `fd9727b6e1217c2f614f9b698455c4ffd82463b4` |
| large  | 2.9 GB | ~3.3 GB | `0f4c8e34f21cf1a914c59d8b3ce882345ad349d6` |

## Quantization

`whisper.cpp` supports integer quantization of the Whisper `ggml` models.
Quantized models require less memory and disk space and depending on the hardware can be processed more efficiently.

Here are the steps for creating and using a quantized model:

```bash
# quantize a model with Q5_0 method
make quantize
./quantize models/ggml-base.en.bin models/ggml-base.en-q5_0.bin q5_0

# run the examples as usual, specifying the quantized model file
./main -m models/ggml-base.en-q5_0.bin ./samples/gb0.wav
```

## Core ML support

On Apple Silicon devices, the Encoder inference can be executed on the Apple Neural Engine (ANE) via Core ML. This can result in significant
speed-up - more than x3 faster compared with CPU-only execution. Here are the instructions for generating a Core ML model and using it with `whisper.cpp`:

- Install Python dependencies needed for the creation of the Core ML model:

  ```bash
  pip install ane_transformers
  pip install openai-whisper
  pip install coremltools
  ```

  - To ensure `coremltools` operates correctly, please confirm that [Xcode](https://developer.apple.com/xcode/) is installed and execute `xcode-select --install` to install the command-line tools.
  - Python 3.10 is recommended.
  - [OPTIONAL] It is recommended to utilize a Python version management system, such as [Miniconda](https://docs.conda.io/en/latest/miniconda.html)  for this step:
    - To create an environment, use: `conda create -n py310-whisper python=3.10 -y`
    - To activate the environment, use: `conda activate py310-whisper`

- Generate a Core ML model. For example, to generate a `base.en` model, use:

  ```bash
  ./models/generate-coreml-model.sh base.en
  ```

  This will generate the folder `models/ggml-base.en-encoder.mlmodelc`

- Build `whisper.cpp` with Core ML support:

  ```bash
  # using Makefile
  make clean
  WHISPER_COREML=1 make -j

  # using CMake
  cd build
  cmake -DWHISPER_COREML=1 ..
  ```

- Run the examples as usual. For example:

  ```bash
  ./main -m models/ggml-base.en.bin -f samples/jfk.wav

  ...

  whisper_init_state: loading Core ML model from 'models/ggml-base.en-encoder.mlmodelc'
  whisper_init_state: first run on a device may take a while ...
  whisper_init_state: Core ML model loaded

  system_info: n_threads = 4 / 10 | AVX = 0 | AVX2 = 0 | AVX512 = 0 | FMA = 0 | NEON = 1 | ARM_FMA = 1 | F16C = 0 | FP16_VA = 1 | WASM_SIMD = 0 | BLAS = 1 | SSE3 = 0 | VSX = 0 | COREML = 1 |

  ...
  ```

  The first run on a device is slow, since the ANE service compiles the Core ML model to some device-specific format.
  Next runs are faster.

For more information about the Core ML implementation please refer to PR [#566](https://github.com/ggerganov/whisper.cpp/pull/566).

## NVIDIA GPU support via cuBLAS

With NVIDIA cards, the Encoder processing can be offloaded to the GPU to a large extend through cuBLAS.
First, make sure you have installed `cuda`: https://developer.nvidia.com/cuda-downloads

Now build `whisper.cpp` with cuBLAS support:

```
make clean
WHISPER_CUBLAS=1 make -j
```

## OpenCL GPU support via CLBlast

For cards and integrated GPUs that support OpenCL, the Encoder processing can be largely offloaded to the GPU through CLBlast. This is especially useful for users with AMD APU's or low end devices for up to ~2x speedup.

First, make sure you have installed `CLBlast` for your OS or Distribution: https://github.com/CNugteren/CLBlast

Now build `whisper.cpp` with CLBlast support:

```
Makefile:
cd whisper.cpp
make clean
WHISPER_CLBLAST=1 make -j

CMake:
cd whisper.cpp ; mkdir build ; cd build
cmake -DWHISPER_CLBLAST=ON  ..
make clean
make -j
cp bin/* ../ 
```


Run all the examples as usual.

## Limitations

- Inference only

## Another example

Here is another example of transcribing a [3:24 min speech](https://upload.wikimedia.org/wikipedia/commons/1/1f/George_W_Bush_Columbia_FINAL.ogg)
in about half a minute on a MacBook M1 Pro, using `medium.en` model:

<details>
  <summary>Expand to see the result</summary>

```java
$ ./main -m models/ggml-medium.en.bin -f samples/gb1.wav -t 8

whisper_init_from_file: loading model from 'models/ggml-medium.en.bin'
whisper_model_load: loading model
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
whisper_model_load: mem required  = 1720.00 MB (+   43.00 MB per decoder)
whisper_model_load: kv self size  =   42.00 MB
whisper_model_load: kv cross size =  140.62 MB
whisper_model_load: adding 1607 extra tokens
whisper_model_load: model ctx     = 1462.35 MB
whisper_model_load: model size    = 1462.12 MB

system_info: n_threads = 8 / 10 | AVX = 0 | AVX2 = 0 | AVX512 = 0 | FMA = 0 | NEON = 1 | ARM_FMA = 1 | F16C = 0 | FP16_VA = 1 | WASM_SIMD = 0 | BLAS = 1 | SSE3 = 0 | VSX = 0 |

main: processing 'samples/gb1.wav' (3179750 samples, 198.7 sec), 8 threads, 1 processors, lang = en, task = transcribe, timestamps = 1 ...


[00:00:00.000 --> 00:00:08.000]   My fellow Americans, this day has brought terrible news and great sadness to our country.
[00:00:08.000 --> 00:00:17.000]   At nine o'clock this morning, Mission Control in Houston lost contact with our Space Shuttle Columbia.
[00:00:17.000 --> 00:00:23.000]   A short time later, debris was seen falling from the skies above Texas.
[00:00:23.000 --> 00:00:29.000]   The Columbia's lost. There are no survivors.
[00:00:29.000 --> 00:00:32.000]   On board was a crew of seven.
[00:00:32.000 --> 00:00:39.000]   Colonel Rick Husband, Lieutenant Colonel Michael Anderson, Commander Laurel Clark,
[00:00:39.000 --> 00:00:48.000]   Captain David Brown, Commander William McCool, Dr. Kultna Shavla, and Ilan Ramon,
[00:00:48.000 --> 00:00:52.000]   a colonel in the Israeli Air Force.
[00:00:52.000 --> 00:00:58.000]   These men and women assumed great risk in the service to all humanity.
[00:00:58.000 --> 00:01:03.000]   In an age when space flight has come to seem almost routine,
[00:01:03.000 --> 00:01:07.000]   it is easy to overlook the dangers of travel by rocket
[00:01:07.000 --> 00:01:12.000]   and the difficulties of navigating the fierce outer atmosphere of the Earth.
[00:01:12.000 --> 00:01:18.000]   These astronauts knew the dangers, and they faced them willingly,
[00:01:18.000 --> 00:01:23.000]   knowing they had a high and noble purpose in life.
[00:01:23.000 --> 00:01:31.000]   Because of their courage and daring and idealism, we will miss them all the more.
[00:01:31.000 --> 00:01:36.000]   All Americans today are thinking as well of the families of these men and women
[00:01:36.000 --> 00:01:40.000]   who have been given this sudden shock and grief.
[00:01:40.000 --> 00:01:45.000]   You're not alone. Our entire nation grieves with you,
[00:01:45.000 --> 00:01:52.000]   and those you love will always have the respect and gratitude of this country.
[00:01:52.000 --> 00:01:56.000]   The cause in which they died will continue.
[00:01:56.000 --> 00:02:04.000]   Mankind is led into the darkness beyond our world by the inspiration of discovery
[00:02:04.000 --> 00:02:11.000]   and the longing to understand. Our journey into space will go on.
[00:02:11.000 --> 00:02:16.000]   In the skies today, we saw destruction and tragedy.
[00:02:16.000 --> 00:02:22.000]   Yet farther than we can see, there is comfort and hope.
[00:02:22.000 --> 00:02:29.000]   In the words of the prophet Isaiah, "Lift your eyes and look to the heavens
[00:02:29.000 --> 00:02:35.000]   who created all these. He who brings out the starry hosts one by one
[00:02:35.000 --> 00:02:39.000]   and calls them each by name."
[00:02:39.000 --> 00:02:46.000]   Because of His great power and mighty strength, not one of them is missing.
[00:02:46.000 --> 00:02:55.000]   The same Creator who names the stars also knows the names of the seven souls we mourn today.
[00:02:55.000 --> 00:03:01.000]   The crew of the shuttle Columbia did not return safely to earth,
[00:03:01.000 --> 00:03:05.000]   yet we can pray that all are safely home.
[00:03:05.000 --> 00:03:13.000]   May God bless the grieving families, and may God continue to bless America.
[00:03:13.000 --> 00:03:19.000]   [Silence]


whisper_print_timings:     fallbacks =   1 p /   0 h
whisper_print_timings:     load time =   569.03 ms
whisper_print_timings:      mel time =   146.85 ms
whisper_print_timings:   sample time =   238.66 ms /   553 runs (    0.43 ms per run)
whisper_print_timings:   encode time = 18665.10 ms /     9 runs ( 2073.90 ms per run)
whisper_print_timings:   decode time = 13090.93 ms /   549 runs (   23.85 ms per run)
whisper_print_timings:    total time = 32733.52 ms
```
</details>

## Real-time audio input example

This is a naive example of performing real-time inference on audio from your microphone.
The [stream](examples/stream) tool samples the audio every half a second and runs the transcription continuously.
More info is available in [issue #10](https://github.com/ggerganov/whisper.cpp/issues/10).

```java
make stream
./stream -m ./models/ggml-base.en.bin -t 8 --step 500 --length 5000
```

https://user-images.githubusercontent.com/1991296/194935793-76afede7-cfa8-48d8-a80f-28ba83be7d09.mp4

## Confidence color-coding

Adding the `--print-colors` argument will print the transcribed text using an experimental color coding strategy
to highlight words with high or low confidence:

```java
./main -m models/ggml-base.en.bin -f samples/gb0.wav --print-colors
```

<img width="965" alt="image" src="https://user-images.githubusercontent.com/1991296/197356445-311c8643-9397-4e5e-b46e-0b4b4daa2530.png">

## Controlling the length of the generated text segments (experimental)

For example, to limit the line length to a maximum of 16 characters, simply add `-ml 16`:

```java
./main -m ./models/ggml-base.en.bin -f ./samples/jfk.wav -ml 16

whisper_model_load: loading model from './models/ggml-base.en.bin'
...
system_info: n_threads = 4 / 10 | AVX2 = 0 | AVX512 = 0 | NEON = 1 | FP16_VA = 1 | WASM_SIMD = 0 | BLAS = 1 |

main: processing './samples/jfk.wav' (176000 samples, 11.0 sec), 4 threads, 1 processors, lang = en, task = transcribe, timestamps = 1 ...

[00:00:00.000 --> 00:00:00.850]   And so my
[00:00:00.850 --> 00:00:01.590]   fellow
[00:00:01.590 --> 00:00:04.140]   Americans, ask
[00:00:04.140 --> 00:00:05.660]   not what your
[00:00:05.660 --> 00:00:06.840]   country can do
[00:00:06.840 --> 00:00:08.430]   for you, ask
[00:00:08.430 --> 00:00:09.440]   what you can do
[00:00:09.440 --> 00:00:10.020]   for your
[00:00:10.020 --> 00:00:11.000]   country.
```

## Word-level timestamp

The `--max-len` argument can be used to obtain word-level timestamps. Simply use `-ml 1`:

```java
./main -m ./models/ggml-base.en.bin -f ./samples/jfk.wav -ml 1

whisper_model_load: loading model from './models/ggml-base.en.bin'
...
system_info: n_threads = 4 / 10 | AVX2 = 0 | AVX512 = 0 | NEON = 1 | FP16_VA = 1 | WASM_SIMD = 0 | BLAS = 1 |

main: processing './samples/jfk.wav' (176000 samples, 11.0 sec), 4 threads, 1 processors, lang = en, task = transcribe, timestamps = 1 ...

[00:00:00.000 --> 00:00:00.320]
[00:00:00.320 --> 00:00:00.370]   And
[00:00:00.370 --> 00:00:00.690]   so
[00:00:00.690 --> 00:00:00.850]   my
[00:00:00.850 --> 00:00:01.590]   fellow
[00:00:01.590 --> 00:00:02.850]   Americans
[00:00:02.850 --> 00:00:03.300]  ,
[00:00:03.300 --> 00:00:04.140]   ask
[00:00:04.140 --> 00:00:04.990]   not
[00:00:04.990 --> 00:00:05.410]   what
[00:00:05.410 --> 00:00:05.660]   your
[00:00:05.660 --> 00:00:06.260]   country
[00:00:06.260 --> 00:00:06.600]   can
[00:00:06.600 --> 00:00:06.840]   do
[00:00:06.840 --> 00:00:07.010]   for
[00:00:07.010 --> 00:00:08.170]   you
[00:00:08.170 --> 00:00:08.190]  ,
[00:00:08.190 --> 00:00:08.430]   ask
[00:00:08.430 --> 00:00:08.910]   what
[00:00:08.910 --> 00:00:09.040]   you
[00:00:09.040 --> 00:00:09.320]   can
[00:00:09.320 --> 00:00:09.440]   do
[00:00:09.440 --> 00:00:09.760]   for
[00:00:09.760 --> 00:00:10.020]   your
[00:00:10.020 --> 00:00:10.510]   country
[00:00:10.510 --> 00:00:11.000]  .
```

## Karaoke-style movie generation (experimental)

The [main](examples/main) example provides support for output of karaoke-style movies, where the
currently pronounced word is highlighted. Use the `-wts` argument and run the generated bash script.
This requires to have `ffmpeg` installed.

Here are a few *"typical"* examples:

```java
./main -m ./models/ggml-base.en.bin -f ./samples/jfk.wav -owts
source ./samples/jfk.wav.wts
ffplay ./samples/jfk.wav.mp4
```

https://user-images.githubusercontent.com/1991296/199337465-dbee4b5e-9aeb-48a3-b1c6-323ac4db5b2c.mp4

---

```java
./main -m ./models/ggml-base.en.bin -f ./samples/mm0.wav -owts
source ./samples/mm0.wav.wts
ffplay ./samples/mm0.wav.mp4
```

https://user-images.githubusercontent.com/1991296/199337504-cc8fd233-0cb7-4920-95f9-4227de3570aa.mp4

---

```java
./main -m ./models/ggml-base.en.bin -f ./samples/gb0.wav -owts
source ./samples/gb0.wav.wts
ffplay ./samples/gb0.wav.mp4
```

https://user-images.githubusercontent.com/1991296/199337538-b7b0c7a3-2753-4a88-a0cd-f28a317987ba.mp4

---

## Video comparison of different models

Use the [extra/bench-wts.sh](https://github.com/ggerganov/whisper.cpp/blob/master/extra/bench-wts.sh) script to generate a video in the following format:

```java
./extra/bench-wts.sh samples/jfk.wav
ffplay ./samples/jfk.wav.all.mp4
```

https://user-images.githubusercontent.com/1991296/223206245-2d36d903-cf8e-4f09-8c3b-eb9f9c39d6fc.mp4

---

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

You can download the converted models using the [models/download-ggml-model.sh](models/download-ggml-model.sh) script
or manually from here:

- https://huggingface.co/ggerganov/whisper.cpp
- https://ggml.ggerganov.com

For more details, see the conversion script [models/convert-pt-to-ggml.py](models/convert-pt-to-ggml.py) or the README
in [models](models).

## [Bindings](https://github.com/ggerganov/whisper.cpp/discussions/categories/bindings)

- [X] Rust: [tazz4843/whisper-rs](https://github.com/tazz4843/whisper-rs) | [#310](https://github.com/ggerganov/whisper.cpp/discussions/310)
- [X] Javascript: [bindings/javascript](bindings/javascript) | [#309](https://github.com/ggerganov/whisper.cpp/discussions/309)
  - React Native (iOS / Android): [whisper.rn](https://github.com/mybigday/whisper.rn)
- [X] Go: [bindings/go](bindings/go) | [#312](https://github.com/ggerganov/whisper.cpp/discussions/312)
- [X] Ruby: [bindings/ruby](bindings/ruby) | [#507](https://github.com/ggerganov/whisper.cpp/discussions/507)
- [X] Objective-C / Swift: [ggerganov/whisper.spm](https://github.com/ggerganov/whisper.spm) | [#313](https://github.com/ggerganov/whisper.cpp/discussions/313)
  - [exPHAT/SwiftWhisper](https://github.com/exPHAT/SwiftWhisper)
- [X] .NET: | [#422](https://github.com/ggerganov/whisper.cpp/discussions/422)
  - [sandrohanea/whisper.net](https://github.com/sandrohanea/whisper.net)
  - [NickDarvey/whisper](https://github.com/NickDarvey/whisper)
- [X] Python: | [#9](https://github.com/ggerganov/whisper.cpp/issues/9)
  - [stlukey/whispercpp.py](https://github.com/stlukey/whispercpp.py) (Cython)
  - [aarnphm/whispercpp](https://github.com/aarnphm/whispercpp) (Pybind11)
- [X] R: [bnosac/audio.whisper](https://github.com/bnosac/audio.whisper)
- [X] Unity: [macoron/whisper.unity](https://github.com/Macoron/whisper.unity)

## Examples

There are various examples of using the library for different projects in the [examples](examples) folder.
Some of the examples are even ported to run in the browser using WebAssembly. Check them out!

| Example | Web | Description |
| ---     | --- | ---         |
| [main](examples/main) | [whisper.wasm](examples/whisper.wasm) | Tool for translating and transcribing audio using Whisper |
| [bench](examples/bench) | [bench.wasm](examples/bench.wasm) | Benchmark the performance of Whisper on your machine |
| [stream](examples/stream) | [stream.wasm](examples/stream.wasm) | Real-time transcription of raw microphone capture |
| [command](examples/command) | [command.wasm](examples/command.wasm) | Basic voice assistant example for receiving voice commands from the mic |
| [talk](examples/talk) | [talk.wasm](examples/talk.wasm) | Talk with a GPT-2 bot |
| [talk-llama](examples/talk-llama) | | Talk with a LLaMA bot |
| [whisper.objc](examples/whisper.objc) | | iOS mobile application using whisper.cpp |
| [whisper.swiftui](examples/whisper.swiftui) | | SwiftUI iOS / macOS application using whisper.cpp |
| [whisper.android](examples/whisper.android) | | Android mobile application using whisper.cpp |
| [whisper.nvim](examples/whisper.nvim) | | Speech-to-text plugin for Neovim |
| [generate-karaoke.sh](examples/generate-karaoke.sh) | | Helper script to easily [generate a karaoke video](https://youtu.be/uj7hVta4blM) of raw audio capture |
| [livestream.sh](examples/livestream.sh) | | [Livestream audio transcription](https://github.com/ggerganov/whisper.cpp/issues/185) |
| [yt-wsp.sh](examples/yt-wsp.sh) | | Download + transcribe and/or translate any VOD [(original)](https://gist.github.com/DaniruKun/96f763ec1a037cc92fe1a059b643b818) |

## [Discussions](https://github.com/ggerganov/whisper.cpp/discussions)

If you have any kind of feedback about this project feel free to use the Discussions section and open a new topic.
You can use the [Show and tell](https://github.com/ggerganov/whisper.cpp/discussions/categories/show-and-tell) category
to share your own projects that use `whisper.cpp`. If you have a question, make sure to check the
[Frequently asked questions (#126)](https://github.com/ggerganov/whisper.cpp/discussions/126) discussion.
