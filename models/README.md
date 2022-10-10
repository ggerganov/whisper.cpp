## Whisper model files in custom ggml format

The [original Whisper PyTorch models provided by OpenAI](https://github.com/openai/whisper/blob/main/whisper/__init__.py#L17-L27)
have been converted to custom `ggml` format in order to be able to load them in C/C++. The conversion has been performed using the
[convert-pt-to-ggml.py](convert-pt-to-ggml.py) script. You can either obtain the original models and generate the `ggml` files
yourself using the conversion script, or you can use the [download-ggml-model.sh](download-ggml-model.sh) script to download the
already converted models from https://ggml.ggerganov.com

Sample usage:

```java
$ ./download-ggml-model.sh base.en
Downloading ggml model base.en ...
models/ggml-base.en.bin          100%[=============================================>] 141.11M  5.41MB/s    in 22s
Done! Model 'base.en' saved in 'models/ggml-base.en.bin'
You can now use it like this:

  $ ./main -m models/ggml-base.en.bin -f samples/jfk.wav
```

A third option to obtain the model files is to download them from Hugging Face:

https://huggingface.co/datasets/ggerganov/whisper.cpp/tree/main

## Model files for testing purposes

The model files pefixed with `for-tests-` are empty (i.e. do not contain any weights) and are used by the CI for testing purposes.
They are directly included in this repository for convenience and the Github Actions CI uses them to run various sanitizer tests.
