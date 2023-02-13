TOPDIR = File.expand_path(File.join(File.dirname(__FILE__), '..'))
EXTDIR = File.join(TOPDIR, 'ext')
#$LIBDIR = File.join(TOPDIR, 'lib')
#$:.unshift(LIBDIR)
$:.unshift(EXTDIR)

require 'whisper'
require 'test/unit'

class TestWhisper < Test::Unit::TestCase
  def setup
    @params  = Whisper::Params.new
    @whisper = Whisper::Context.new(File.join(TOPDIR, '..', '..', 'models', 'for-tests-ggml-base.en.bin'))
  end

  def test_autodetect
    @params.auto_detection = true
  end

  def test_whisper
  end

end
