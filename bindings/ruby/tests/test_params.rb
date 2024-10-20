require 'whisper'

class TestParams < Test::Unit::TestCase
  def setup
    @params  = Whisper::Params.new
  end

  def test_language
    @params.language = "en"
    assert_equal @params.language, "en"
    @params.language = "auto"
    assert_equal @params.language, "auto"
  end

  def test_offset
    @params.offset = 10_000
    assert_equal @params.offset, 10_000
    @params.offset = 0
    assert_equal @params.offset, 0
  end

  def test_duration
    @params.duration = 60_000
    assert_equal @params.duration, 60_000
    @params.duration = 0
    assert_equal @params.duration, 0
  end

  def test_max_text_tokens
    @params.max_text_tokens = 300
    assert_equal @params.max_text_tokens, 300
    @params.max_text_tokens = 0
    assert_equal @params.max_text_tokens, 0
  end

  def test_translate
    @params.translate = true
    assert @params.translate
    @params.translate = false
    assert !@params.translate
  end

  def test_no_context
    @params.no_context = true
    assert @params.no_context
    @params.no_context = false
    assert !@params.no_context
  end

  def test_single_segment
    @params.single_segment = true
    assert @params.single_segment
    @params.single_segment = false
    assert !@params.single_segment
  end

  def test_print_special
    @params.print_special = true
    assert @params.print_special
    @params.print_special = false
    assert !@params.print_special
  end

  def test_print_progress
    @params.print_progress = true
    assert @params.print_progress
    @params.print_progress = false
    assert !@params.print_progress
  end

  def test_print_realtime
    @params.print_realtime = true
    assert @params.print_realtime
    @params.print_realtime = false
    assert !@params.print_realtime
  end

  def test_print_timestamps
    @params.print_timestamps = true
    assert @params.print_timestamps
    @params.print_timestamps = false
    assert !@params.print_timestamps
  end

  def test_suppress_blank
    @params.suppress_blank = true
    assert @params.suppress_blank
    @params.suppress_blank = false
    assert !@params.suppress_blank
  end

  def test_suppress_non_speech_tokens
    @params.suppress_non_speech_tokens = true
    assert @params.suppress_non_speech_tokens
    @params.suppress_non_speech_tokens = false
    assert !@params.suppress_non_speech_tokens
  end

  def test_token_timestamps
    @params.token_timestamps = true
    assert @params.token_timestamps
    @params.token_timestamps = false
    assert !@params.token_timestamps
  end

  def test_split_on_word
    @params.split_on_word = true
    assert @params.split_on_word
    @params.split_on_word = false
    assert !@params.split_on_word
  end
end
