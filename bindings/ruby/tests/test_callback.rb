require "test/unit"
require "whisper"

class TestCallback < Test::Unit::TestCase
  TOPDIR = File.expand_path(File.join(File.dirname(__FILE__), '..'))

  def setup
    @params = Whisper::Params.new
    @whisper = Whisper::Context.new(File.join(TOPDIR, '..', '..', 'models', 'ggml-base.en.bin'))
    @audio = File.join(TOPDIR, '..', '..', 'samples', 'jfk.wav')
  end

  def test_new_segment_callback
    @params.new_segment_callback = ->(context, state, n_new, user_data) {
      assert_kind_of Integer, n_new
      assert n_new > 0
      assert_same @whisper, context

      n_segments = context.full_n_segments
      n_new.times do |i|
        i_segment = n_segments - 1 + i
        start_time = context.full_get_segment_t0(i_segment) * 10
        end_time = context.full_get_segment_t1(i_segment) * 10
        text = context.full_get_segment_text(i_segment)

        assert_kind_of Integer, start_time
        assert start_time >= 0
        assert_kind_of Integer, end_time
        assert end_time > 0
        assert_match /ask not what your country can do for you, ask what you can do for your country/, text if i_segment == 0
      end
    }

    @whisper.transcribe(@audio, @params)
  end

  def test_new_segment_callback_closure
    search_word = "what"
    @params.new_segment_callback = ->(context, state, n_new, user_data) {
      n_segments = context.full_n_segments
      n_new.times do |i|
        i_segment = n_segments - 1 + i
        text = context.full_get_segment_text(i_segment)
        if text.include?(search_word)
          t0 = context.full_get_segment_t0(i_segment)
          t1 = context.full_get_segment_t1(i_segment)
          raise "search word '#{search_word}' found at between #{t0} and #{t1}"
        end
      end
    }

    assert_raise RuntimeError do
      @whisper.transcribe(@audio, @params)
    end
  end

  def test_new_segment_callback_user_data
    udata = Object.new
    @params.new_segment_callback_user_data = udata
    @params.new_segment_callback = ->(context, state, n_new, user_data) {
      assert_same udata, user_data
    }

    @whisper.transcribe(@audio, @params)
  end

  def test_new_segment_callback_user_data_gc
    @params.new_segment_callback_user_data = "My user data"
    @params.new_segment_callback = ->(context, state, n_new, user_data) {
      assert_equal "My user data", user_data
    }
    GC.start

    assert_same @whisper, @whisper.transcribe(@audio, @params)
  end
end
