require 'whisper'
require 'test/unit'

class TestWhisper < Test::Unit::TestCase
  TOPDIR = File.expand_path(File.join(File.dirname(__FILE__), '..'))

  def setup
    @params  = Whisper::Params.new
  end

  def test_whisper
    @whisper = Whisper::Context.new(File.join(TOPDIR, '..', '..', 'models', 'ggml-base.en.bin'))
    params  = Whisper::Params.new
    params.print_timestamps = false

    jfk = File.join(TOPDIR, '..', '..', 'samples', 'jfk.wav')
    @whisper.transcribe(jfk, params) {|text|
      assert_match /ask not what your country can do for you, ask what you can do for your country/, text
    }
  end

  sub_test_case "After transcription" do
    class << self
      attr_reader :whisper

      def startup
        @whisper = Whisper::Context.new(File.join(TOPDIR, '..', '..', 'models', 'ggml-base.en.bin'))
        params = Whisper::Params.new
        params.print_timestamps = false
        jfk = File.join(TOPDIR, '..', '..', 'samples', 'jfk.wav')
        @whisper.transcribe(jfk, params)
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
end
