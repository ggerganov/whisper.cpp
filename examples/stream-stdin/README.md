# stream-stdin

This is a naive example of performing real-time inference on audio
from your microphone.  The `stream-stdin` tool samples the audio every
half a second and runs the transcription continously, just like the
`stream` example.  Only it doesn't need to be compiled with SDL to do
it.

```shell
$ ffmpeg -i capture.wav -acodec pcm_s16le -f s16le -ac 1 -ar 16000 - | ./stream -m ./models/ggml-base.en.bin -t 8 --step 500 --length 5000
```

It expects raw, mono audio on stdin.  Because it's raw it can't tell
what the format is to convert it from anything else.

## Why this matters

Because you can stream the audio over the network with `ffmpeg`'s
`rtp` support.  Or you can just use `netcat`.

## Building

$ `make stream-stdin`
