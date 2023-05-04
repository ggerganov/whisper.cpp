## Whisper model files in custom ggml format

The [original Whisper PyTorch models provided by OpenAI](https://github.com/openai/whisper/blob/main/whisper/__init__.py#L17-L27)
are converted to custom `ggml` format in order to be able to load them in C/C++.
Conversion is performed using the [convert-pt-to-ggml.py](convert-pt-to-ggml.py) script.

You can either obtain the original models and generate the `ggml` files yourself using the conversion script,
or you can use the [download-ggml-model.sh](download-ggml-model.sh) script to download the already converted models.
Currently, they are hosted on the following locations:

- https://huggingface.co/ggerganov/whisper.cpp
- https://ggml.ggerganov.com

Sample download:

```java
$ ./download-ggml-model.sh base.en
Downloading ggml model base.en ...
models/ggml-base.en.bin          100%[=============================================>] 141.11M  5.41MB/s    in 22s
Done! Model 'base.en' saved in 'models/ggml-base.en.bin'
You can now use it like this:

  $ ./main -m models/ggml-base.en.bin -f samples/jfk.wav
```

To convert the files yourself, use the convert-pt-to-ggml.py script. Here is an example usage.
The original PyTorch files are assumed to have been downloaded into ~/.cache/whisper
Change `~/path/to/repo/whisper/` to the location for your copy of the Whisper source:
```
mkdir models/whisper-medium
python models/convert-pt-to-ggml.py ~/.cache/whisper/medium.pt ~/path/to/repo/whisper/ ./models/whisper-medium
mv ./models/whisper-medium/ggml-model.bin models/ggml-medium.bin
rmdir models/whisper-medium
```

A third option to obtain the model files is to download them from Hugging Face:

https://huggingface.co/ggerganov/whisper.cpp/tree/main

## Available models

| Model     | Disk   | Mem     | SHA                                        |
| ---       | ---    | ---     | ---                                        |
| tiny      |  75 MB | ~390 MB | `bd577a113a864445d4c299885e0cb97d4ba92b5f` |
| tiny.en   |  75 MB | ~390 MB | `c78c86eb1a8faa21b369bcd33207cc90d64ae9df` |
| base      | 142 MB | ~500 MB | `465707469ff3a37a2b9b8d8f89f2f99de7299dac` |
| base.en   | 142 MB | ~500 MB | `137c40403d78fd54d454da0f9bd998f78703390c` |
| small     | 466 MB | ~1.0 GB | `55356645c2b361a969dfd0ef2c5a50d530afd8d5` |
| small.en  | 466 MB | ~1.0 GB | `db8a495a91d927739e50b3fc1cc4c6b8f6c2d022` |
| medium    | 1.5 GB | ~2.6 GB | `fd9727b6e1217c2f614f9b698455c4ffd82463b4` |
| medium.en | 1.5 GB | ~2.6 GB | `8c30f0e44ce9560643ebd10bbe50cd20eafd3723` |
| large-v1  | 2.9 GB | ~4.7 GB | `b1caaf735c4cc1429223d5a74f0f4d0b9b59a299` |
| large     | 2.9 GB | ~4.7 GB | `0f4c8e34f21cf1a914c59d8b3ce882345ad349d6` |

## Model files for testing purposes

The model files prefixed with `for-tests-` are empty (i.e. do not contain any weights) and are used by the CI for
testing purposes. They are directly included in this repository for convenience and the Github Actions CI uses them to
run various sanitizer tests.

## Fine-tuned models

There are community efforts for creating fine-tuned Whisper models using extra training data. For example, this
[blog post](https://huggingface.co/blog/fine-tune-whisper) describes a method for fine-tuning using Hugging Face (HF)
Transformer implementation of Whisper. The produced models are in slightly different format compared to the original
OpenAI format. To read the HF models you can use the [convert-h5-to-ggml.py](convert-h5-to-ggml.py) script like this:

```bash
git clone https://github.com/openai/whisper
git clone https://github.com/ggerganov/whisper.cpp

# clone HF fine-tuned model (this is just an example)
git clone https://huggingface.co/openai/whisper-base.en

# convert the model to ggml
python3 ./whisper.cpp/models/convert-h5-to-ggml.py ./whisper-medium/ ./whisper .
```
