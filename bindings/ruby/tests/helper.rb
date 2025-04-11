require "test/unit"
require "whisper"
require_relative "jfk_reader/jfk_reader"

puts "ldd lib/whisper.so"
puts `ldd lib/whisper.so`
puts "nm lib/whisper.so"
puts `nm lib/whisper.so`

class TestBase < Test::Unit::TestCase
  AUDIO = File.join(__dir__, "..", "..", "..", "samples", "jfk.wav")

  class << self
    attr_reader :whisper

    def startup
      @whisper = Whisper::Context.new("base.en")
      params = Whisper::Params.new
      params.print_timestamps = false
      @whisper.transcribe(TestBase::AUDIO, params)
    end
  end

  private

  def whisper
    self.class.whisper
  end
end
