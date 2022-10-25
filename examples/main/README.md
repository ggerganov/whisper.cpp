# main

This is the main example demonstrating most of the functionality of the Whisper model.
It can be used as a reference for using the `whisper.cpp` library in other projects.

```
./main -h

usage: ./main [options] file0.wav file1.wav ...

options:
  -h,       --help           show this help message and exit
  -s SEED,  --seed SEED      RNG seed (default: -1)
  -t N,     --threads N      number of threads to use during computation (default: 4)
  -o N,     --offset N       offset in milliseconds (default: 0)
  -v,       --verbose        verbose output
            --translate      translate from source language to english
  -otxt,    --output-txt     output result in a text file
  -ovtt,    --output-vtt     output result in a vtt file
  -osrt,    --output-srt     output result in a srt file
  -ps,      --print_special  print special tokens
  -nt,      --no_timestamps  do not print timestamps
  -l LANG,  --language LANG  spoken language (default: en)
  -m FNAME, --model FNAME    model path (default: models/ggml-base.en.bin)
  -f FNAME, --file FNAME     input WAV file path
```
