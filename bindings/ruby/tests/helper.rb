require "test/unit"
require "whisper"

class TestBase < Test::Unit::TestCase
  MODEL = File.join(__dir__, "..", "..", "..", "models", "ggml-base.en.bin")
  AUDIO = File.join(__dir__, "..", "..", "..", "samples", "jfk.wav")
end
