require_relative "helper"
require "stringio"

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
      assert_equal Whisper::LOG_LEVEL_INFO, log[0]
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
end
