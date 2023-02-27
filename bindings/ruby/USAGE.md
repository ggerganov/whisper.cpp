# Ruby Guide

We expose Whisper::Context and Whisper::Params.  The Context object can be used to transcribe.
Parameters can be set on the Params object to customize how the transcription is generated.

```
  require 'whisper'
  whisper = Whisper::Context.new('ggml-base.en.bin')
  params  = Whisper::Params.new
  whisper.transcribe('jfk.wav', params) {|text|
    assert_match /ask not what your country can do for you, ask what you can do for your country/, text
  }
```
