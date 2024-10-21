require "test/unit"
require "whisper"

class TestSegment < Test::Unit::TestCase
  TOPDIR = File.expand_path(File.join(File.dirname(__FILE__), '..'))

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

  private

  def whisper
    self.class.whisper
  end
end
