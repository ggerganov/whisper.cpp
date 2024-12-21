require_relative "helper"

class TestSegment < TestBase
  def test_iteration
    whisper.each_segment do |segment|
      assert_instance_of Whisper::Segment, segment
    end
  end

  def test_enumerator
    enum = whisper.each_segment
    assert_instance_of Enumerator, enum
    enum.to_a.each_with_index do |segment, index|
      assert_instance_of Whisper::Segment, segment
      assert_kind_of Integer, index
    end
  end

  def test_start_time
    i = 0
    whisper.each_segment do |segment|
      assert_equal 0, segment.start_time if i == 0
      i += 1
    end
  end

  def test_end_time
    i = 0
    whisper.each_segment do |segment|
      assert_equal whisper.full_get_segment_t1(i) * 10, segment.end_time
      i += 1
    end
  end

  def test_no_speech_prob
    no_speech_prob = nil
    whisper.each_segment do |segment|
      no_speech_prob = segment.no_speech_prob
    end
    assert no_speech_prob > 0.0
  end

  def test_on_new_segment
    params = Whisper::Params.new
    seg = nil
    index = 0
    params.on_new_segment do |segment|
      assert_instance_of Whisper::Segment, segment
      if index == 0
        seg = segment
        assert_equal 0, segment.start_time
        assert_match /ask not what your country can do for you, ask what you can do for your country/, segment.text
      end
      index += 1
    end
    whisper.transcribe(AUDIO, params)
    assert_equal 0, seg.start_time
    assert_match /ask not what your country can do for you, ask what you can do for your country/, seg.text
  end

  def test_on_new_segment_twice
    params = Whisper::Params.new
    seg = nil
    params.on_new_segment do |segment|
      seg = segment
      return
    end
    params.on_new_segment do |segment|
      assert_same seg, segment
      return
    end
    whisper.transcribe(AUDIO, params)
  end
end
