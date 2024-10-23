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

### API ###

Once `Whisper::Context#transcribe` called, you can retrieve segments by `#each_segment`:

```ruby
def format_time(time_ms)
  sec, decimal_part = time_ms.divmod(1000)
  min, sec = sec.divmod(60)
  hour, min = min.divmod(60)
  "%02d:%02d:%02d.%03d" % [hour, min, sec, decimal_part]
end

whisper.transcribe("path/to/audio.wav", params)

whisper.each_segment.with_index do |segment, index|
  line = "[%{nth}: %{st} --> %{ed}] %{text}" % {
    nth: index + 1,
    st: format_time(segment.start_time),
    ed: format_time(segment.end_time),
    text: segment.text
  }
  line << " (speaker turned)" if segment.speaker_next_turn?
  puts line
end

```

You can also add hook to params called on new segment:

```ruby
def format_time(time_ms)
  sec, decimal_part = time_ms.divmod(1000)
  min, sec = sec.divmod(60)
  hour, min = min.divmod(60)
  "%02d:%02d:%02d.%03d" % [hour, min, sec, decimal_part]
end

# Add hook before calling #transcribe
params.on_new_segment do |segment|
  line = "[%{st} --> %{ed}] %{text}" % {
    st: format_time(segment.start_time),
    ed: format_time(segment.end_time),
    text: segment.text
  }
  line << " (speaker turned)" if segment.speaker_next_turn?
  puts line
end

whisper.transcribe("path/to/audio.wav", params)

```

[whisper.cpp]: https://github.com/ggerganov/whisper.cpp
[models]: https://github.com/ggerganov/whisper.cpp/tree/master/models
