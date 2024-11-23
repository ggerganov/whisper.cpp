require_relative "helper"
require "stringio"
require "etc"

# Exists to detect memory-related bug
Whisper.log_set ->(level, buffer, user_data) {}, nil

class TestWhisper < TestBase
  def setup
    @params  = Whisper::Params.new
  end

  def test_whisper
    @whisper = Whisper::Context.new(MODEL)
    params  = Whisper::Params.new
    params.print_timestamps = false

    @whisper.transcribe(AUDIO, params) {|text|
      assert_match /ask not what your country can do for you, ask what you can do for your country/, text
    }
  end

  sub_test_case "After transcription" do
    class << self
      attr_reader :whisper

      def startup
        @whisper = Whisper::Context.new(TestBase::MODEL)
        params = Whisper::Params.new
        params.print_timestamps = false
        @whisper.transcribe(TestBase::AUDIO, params)
      end
    end

    def whisper
      self.class.whisper
    end

    def test_full_n_segments
      assert_equal 1, whisper.full_n_segments
    end

    def test_full_lang_id
      assert_equal 0, whisper.full_lang_id
    end

    def test_full_get_segment_t0
      assert_equal 0, whisper.full_get_segment_t0(0)
      assert_raise IndexError do
        whisper.full_get_segment_t0(whisper.full_n_segments)
      end
      assert_raise IndexError do
        whisper.full_get_segment_t0(-1)
      end
    end

    def test_full_get_segment_t1
      t1 = whisper.full_get_segment_t1(0)
      assert_kind_of Integer, t1
      assert t1 > 0
      assert_raise IndexError do
        whisper.full_get_segment_t1(whisper.full_n_segments)
      end
    end

    def test_full_get_segment_speaker_turn_next
      assert_false whisper.full_get_segment_speaker_turn_next(0)
    end

    def test_full_get_segment_text
      assert_match /ask not what your country can do for you, ask what you can do for your country/, whisper.full_get_segment_text(0)
    end
  end

  def test_lang_max_id
    assert_kind_of Integer, Whisper.lang_max_id
  end

  def test_lang_id
    assert_equal 0, Whisper.lang_id("en")
    assert_raise ArgumentError do
      Whisper.lang_id("non existing language")
    end
  end

  def test_lang_str
    assert_equal "en", Whisper.lang_str(0)
    assert_raise IndexError do
      Whisper.lang_str(Whisper.lang_max_id + 1)
    end
  end

  def test_lang_str_full
    assert_equal "english", Whisper.lang_str_full(0)
    assert_raise IndexError do
      Whisper.lang_str_full(Whisper.lang_max_id + 1)
    end
  end

  def test_log_set
    user_data = Object.new
    logs = []
    log_callback = ->(level, buffer, udata) {
      logs << [level, buffer, udata]
    }
    Whisper.log_set log_callback, user_data
    Whisper::Context.new(MODEL)

    assert logs.length > 30
    logs.each do |log|
      assert_include [Whisper::LOG_LEVEL_DEBUG, Whisper::LOG_LEVEL_INFO, Whisper::LOG_LEVEL_WARN], log[0]
      assert_same user_data, log[2]
    end
  end

  def test_log_suppress
    stderr = $stderr
    Whisper.log_set ->(level, buffer, user_data) {
      # do nothing
    }, nil
    dev = StringIO.new("")
    $stderr = dev
    Whisper::Context.new(MODEL)
    assert_empty dev.string
  ensure
    $stderr = stderr
  end

  sub_test_case "full" do
    def setup
      super
      @whisper = Whisper::Context.new(MODEL)
      @samples = File.read(AUDIO, nil, 78).unpack("s<*").collect {|i| i.to_f / 2**15}
    end

    def test_full
      @whisper.full(@params, @samples, @samples.length)

      assert_equal 1, @whisper.full_n_segments
      assert_match /ask not what your country can do for you, ask what you can do for your country/, @whisper.each_segment.first.text
    end

    def test_full_without_length
      @whisper.full(@params, @samples)

      assert_equal 1, @whisper.full_n_segments
      assert_match /ask not what your country can do for you, ask what you can do for your country/, @whisper.each_segment.first.text
    end

    def test_full_enumerator
      samples = @samples.each
      @whisper.full(@params, samples, @samples.length)

      assert_equal 1, @whisper.full_n_segments
      assert_match /ask not what your country can do for you, ask what you can do for your country/, @whisper.each_segment.first.text
    end

    def test_full_enumerator_without_length
      samples = @samples.each
      assert_raise ArgumentError do
        @whisper.full(@params, samples)
      end
    end

    def test_full_enumerator_with_too_large_length
      samples = @samples.each.take(10).to_enum
      assert_raise StopIteration do
        @whisper.full(@params, samples, 11)
      end
    end

    def test_full_with_memory_view
      samples = JFKReader.new(AUDIO)
      @whisper.full(@params, samples)

      assert_equal 1, @whisper.full_n_segments
      assert_match /ask not what your country can do for you, ask what you can do for your country/, @whisper.each_segment.first.text
    end

    def test_full_parallel
      @whisper.full_parallel(@params, @samples, @samples.length, Etc.nprocessors)

      assert_equal Etc.nprocessors, @whisper.full_n_segments
      text = @whisper.each_segment.collect(&:text).join
      assert_match /ask what you can do/i, text
      assert_match /for your country/i, text
    end

    def test_full_parallel_with_memory_view
      samples = JFKReader.new(AUDIO)
      @whisper.full_parallel(@params, samples, nil, Etc.nprocessors)

      assert_equal Etc.nprocessors, @whisper.full_n_segments
      text = @whisper.each_segment.collect(&:text).join
      assert_match /ask what you can do/i, text
      assert_match /for your country/i, text
    end

    def test_full_parallel_without_length_and_n_processors
      @whisper.full_parallel(@params, @samples)

      assert_equal 1, @whisper.full_n_segments
      text = @whisper.each_segment.collect(&:text).join
      assert_match /ask what you can do/i, text
      assert_match /for your country/i, text
    end

    def test_full_parallel_without_length
      @whisper.full_parallel(@params, @samples, nil, Etc.nprocessors)

      assert_equal Etc.nprocessors, @whisper.full_n_segments
      text = @whisper.each_segment.collect(&:text).join
      assert_match /ask what you can do/i, text
      assert_match /for your country/i, text
    end

    def test_full_parallel_without_n_processors
      @whisper.full_parallel(@params, @samples, @samples.length)

      assert_equal 1, @whisper.full_n_segments
      text = @whisper.each_segment.collect(&:text).join
      assert_match /ask what you can do/i, text
      assert_match /for your country/i, text
    end
  end
end
