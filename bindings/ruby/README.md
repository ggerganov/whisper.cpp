whispercpp
==========

![whisper.cpp](https://user-images.githubusercontent.com/1991296/235238348-05d0f6a4-da44-4900-a1de-d0707e75b763.jpeg)

Ruby bindings for [whisper.cpp][], an interface of automatic speech recognition model.

Installation
------------

Install the gem and add to the application's Gemfile by executing:

    $ bundle add whispercpp

If bundler is not being used to manage dependencies, install the gem by executing:

    $ gem install whispercpp

Usage
-----

NOTE: This gem is still in development. API is not stable for now.

```ruby
require "whisper"

whisper = Whisper::Context.new("path/to/model.bin")

params = Whisper::Params.new
params.language = "en"
params.offset = 10_000
params.duration = 60_000
params.max_text_tokens = 300
params.translate = true
params.print_timestamps = false
params.new_segment_callback = ->(output, t0, t1, index) {
  puts "segment #{index}: #{t0}ms -> #{t1}ms: #{output}"
}

whisper.transcribe("path/to/audio.wav", params) do |whole_text|
  puts whole_text
end

```

### Preparing model ###

Use script to download model file(s):

```bash
git clone https://github.com/ggerganov/whisper.cpp.git
cd whisper.cpp
sh ./models/download-ggml-model.sh base.en
```

There are some types of models. See [models][] page for details.

### Preparing audio file ###

Currently, whisper.cpp accepts only 16-bit WAV files.

[whisper.cpp]: https://github.com/ggerganov/whisper.cpp
[models]: https://github.com/ggerganov/whisper.cpp/tree/master/models
