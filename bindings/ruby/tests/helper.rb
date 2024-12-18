require "test/unit"
require "whisper"
require_relative "jfk_reader/jfk_reader"

class TestBase < Test::Unit::TestCase
  AUDIO = File.join(__dir__, "..", "..", "..", "samples", "jfk.wav")
end
