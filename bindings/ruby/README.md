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

whisper = Whisper::Context.new("base")

params = Whisper::Params.new
params.language = "en"
params.offset = 10_000
params.duration = 60_000
params.max_text_tokens = 300
params.translate = true
params.print_timestamps = false
params.initial_prompt = "Initial prompt here."

whisper.transcribe("path/to/audio.wav", params) do |whole_text|
  puts whole_text
end

```

### Preparing model ###

Some models are prepared up-front:

```ruby
base_en = Whisper::Model.pre_converted_models["base.en"]
whisper = Whisper::Context.new(base_en)
```

At first time you use a model, it is downloaded automatically. After that, downloaded cached file is used. To clear cache, call `#clear_cache`:

```ruby
Whisper::Model.pre_converted_models["base"].clear_cache
```

You also can use shorthand for pre-converted models:

```ruby
whisper = Whisper::Context.new("base.en")
```

You can see the list of prepared model names by `Whisper::Model.pre_converted_models.keys`:

```ruby
puts Whisper::Model.pre_converted_models.keys
# tiny
# tiny.en
# tiny-q5_1
# tiny.en-q5_1
# tiny-q8_0
# base
# base.en
# base-q5_1
# base.en-q5_1
# base-q8_0
#   :
#   :
```

You can also use local model files you prepared:

```ruby
whisper = Whisper::Context.new("path/to/your/model.bin")
```

Or, you can download model files:

```ruby
whisper = Whisper::Context.new("https://example.net/uri/of/your/model.bin")
# Or
whisper = Whisper::Context.new(URI("https://example.net/uri/of/your/model.bin"))
```

See [models][] page for details.

### Preparing audio file ###

Currently, whisper.cpp accepts only 16-bit WAV files.

API
---

### Segments ###

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

### Models ###

You can see model information:

```ruby
whisper = Whisper::Context.new("base")
model = whisper.model

model.n_vocab # => 51864
model.n_audio_ctx # => 1500
model.n_audio_state # => 512
model.n_audio_head # => 8
model.n_audio_layer # => 6
model.n_text_ctx # => 448
model.n_text_state # => 512
model.n_text_head # => 8
model.n_text_layer # => 6
model.n_mels # => 80
model.ftype # => 1
model.type # => "base"

```

### Logging ###

You can set log callback:

```ruby
prefix = "[MyApp] "
log_callback = ->(level, buffer, user_data) {
  case level
  when Whisper::LOG_LEVEL_NONE
    puts "#{user_data}none: #{buffer}"
  when Whisper::LOG_LEVEL_INFO
    puts "#{user_data}info: #{buffer}"
  when Whisper::LOG_LEVEL_WARN
    puts "#{user_data}warn: #{buffer}"
  when Whisper::LOG_LEVEL_ERROR
    puts "#{user_data}error: #{buffer}"
  when Whisper::LOG_LEVEL_DEBUG
    puts "#{user_data}debug: #{buffer}"
  when Whisper::LOG_LEVEL_CONT
    puts "#{user_data}same to previous: #{buffer}"
  end
}
Whisper.log_set log_callback, prefix
```

Using this feature, you are also able to suppress log:

```ruby
Whisper.log_set ->(level, buffer, user_data) {
  # do nothing
}, nil
Whisper::Context.new("base")
```

### Low-level API to transcribe ###

You can also call `Whisper::Context#full` and `#full_parallel` with a Ruby array as samples. Although `#transcribe` with audio file path is recommended because it extracts PCM samples in C++ and is fast, `#full` and `#full_parallel` give you flexibility.

```ruby
require "whisper"
require "wavefile"

reader = WaveFile::Reader.new("path/to/audio.wav", WaveFile::Format.new(:mono, :float, 16000))
samples = reader.enum_for(:each_buffer).map(&:samples).flatten

whisper = Whisper::Context.new("base")
whisper.full(Whisper::Params.new, samples)
whisper.each_segment do |segment|
  puts segment.text
end
```

The second argument `samples` may be an array, an object with `length` and `each` method, or a MemoryView. If you can prepare audio data as C array and export it as a MemoryView, whispercpp accepts and works with it with zero copy.

Development
-----------

    % git clone https://github.com/ggerganov/whisper.cpp.git
    % cd whisper.cpp/bindings/ruby
    % rake test

First call of `rake test` builds an extension and downloads a model for testing. After that, you add tests in `tests` directory and modify `ext/ruby_whisper.cpp`.

If something seems wrong on build, running `rake clean` solves some cases.

License
-------

The same to [whisper.cpp][].

[whisper.cpp]: https://github.com/ggerganov/whisper.cpp
[models]: https://github.com/ggerganov/whisper.cpp/tree/master/models
