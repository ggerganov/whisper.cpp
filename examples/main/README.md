# main

This is the main example demonstrating most of the functionality of the Whisper model.
It can be used as a reference for using the `whisper.cpp` library in other projects.

```
./main -h

usage: ./bin/main [options] file0.wav file1.wav ...

  -h,       --help           show this help message and exit
  -s SEED,  --seed SEED      RNG seed (default: -1)
  -t N,     --threads N      number of threads to use during computation (default: 4)
  -p N,     --processors N   number of processors to use during computation (default: 1)
  -ot N,    --offset-t N     time offset in milliseconds (default: 0)
  -on N,    --offset-n N     segment index offset (default: 0)
  -mc N,    --max-context N  maximum number of text context tokens to store (default: max)
  -ml N,    --max-len N      maximum segment length in characters (default: 0)
  -wt N,    --word-thold N   word timestamp probability threshold (default: 0.010000)
  -v,       --verbose        verbose output
            --translate      translate from source language to english
  -otxt,    --output-txt     output result in a text file
  -ovtt,    --output-vtt     output result in a vtt file
  -osrt,    --output-srt     output result in a srt file
  -owts,    --output-words   output script for generating karaoke video
  -ps,      --print_special  print special tokens
  -pc,      --print_colors   print colors
  -nt,      --no_timestamps  do not print timestamps
  -l LANG,  --language LANG  spoken language (default: en)
  -m FNAME, --model FNAME    model path (default: models/ggml-base.en.bin)
  -f FNAME, --file FNAME     input WAV file path
  -h,       --help           show this help message and exit

```
