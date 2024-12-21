require_relative "helper"

class TestParams < TestBase
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

  def test_suppress_nst
    @params.suppress_nst = true
    assert @params.suppress_nst
    @params.suppress_nst = false
    assert !@params.suppress_nst
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

  def test_initial_prompt
    assert_nil @params.initial_prompt
    @params.initial_prompt = "You are a polite person."
    assert_equal "You are a polite person.", @params.initial_prompt
  end

  def test_temperature
    assert_equal 0.0, @params.temperature
    @params.temperature = 0.5
    assert_equal 0.5, @params.temperature
  end

  def test_max_initial_ts
    assert_equal 1.0, @params.max_initial_ts
    @params.max_initial_ts = 600.0
    assert_equal 600.0, @params.max_initial_ts
  end

  def test_length_penalty
    assert_equal -1.0, @params.length_penalty
    @params.length_penalty = 0.5
    assert_equal 0.5, @params.length_penalty
  end

  def test_temperature_inc
    assert_in_delta 0.2, @params.temperature_inc
    @params.temperature_inc = 0.5
    assert_in_delta 0.5, @params.temperature_inc
  end

  def test_entropy_thold
    assert_in_delta 2.4, @params.entropy_thold
    @params.entropy_thold = 3.0
    assert_in_delta 3.0, @params.entropy_thold
  end

  def test_logprob_thold
    assert_in_delta -1.0, @params.logprob_thold
    @params.logprob_thold = -0.5
    assert_in_delta -0.5, @params.logprob_thold
  end

  def test_no_speech_thold
    assert_in_delta 0.6, @params.no_speech_thold
    @params.no_speech_thold = 0.2
    assert_in_delta 0.2, @params.no_speech_thold
  end
end
