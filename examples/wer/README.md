# whisper.cpp/examples/wer

This is a command line tool for calculating the Word Error Rate (WER). This tool
expects that reference transcriptions (the known correct transcriptions)
and acutual transcriptions from whisper.cpp are available in two separate
directories where the file names are the identical.

### Usage
```console
$ ./build/bin/whisper-wer
Usage: ./build/bin/whisper-wer [options]
Options:
  -r, --reference PATH   Full path to reference transcriptions directory
  -a, --actual PATH      Full path to actual transcriptions directory
  --help                 Display this help message
```

### Example Usage with whisper-cli
First, generate transcription(s) using whisper-cli:
```
./build/bin/whisper-cli \
	-m models/ggml-base.en.bin \
	-f samples/jfk.wav \
	--output-txt
    ...
    output_txt: saving output to 'samples/jfk.wav.txt'
```
Next, copy the transcription to a directory where the actual transcriptions
are stored. In this example we will use a directory called `actual_transcriptions`
in this examples directory:
```console
$ cp samples/jfk.wav.txt examples/wer/actual_transcriptions
```
In a real world scenario the reference transcriptions would be available
representing the known correct text. In this case we have already placed a file
in `examples/wer/reference_transcriptions` that can be used for testing, where
only a single word was changed (`Americans` -> `Swedes`).

Finally, run the whisper-wer tool:
```console
$ ./build/bin/whisper-wer -r examples/wer/reference_transcriptions/ -a examples/wer/actual_transcriptions/
Word Error Rate for : jfk.wav.txt
  Reference words: 22
  Actual words: 22
  Substitutions: 1
  Deletions: 0
  Insertions: 0
  Total edits: 1
  WER: 0.045455
```

