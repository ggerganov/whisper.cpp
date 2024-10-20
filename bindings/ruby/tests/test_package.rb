require 'test/unit'
require 'tempfile'
require 'tmpdir'
require 'shellwords'

class TestPackage < Test::Unit::TestCase
  def test_build
    Tempfile.create do |file|
      assert system("gem", "build", "whispercpp.gemspec", "--output", file.to_path.shellescape, exception: true)
      assert file.size > 0
    end
  end

  sub_test_case "Building binary on installation" do
    def setup
      system "rake", "build", exception: true
    end

    def test_install
      filename = `rake -Tbuild`.match(/(whispercpp-(?:.+)\.gem)/)[1]
      basename = "whisper.#{RbConfig::CONFIG["DLEXT"]}"
      Dir.mktmpdir do |dir|
        system "gem", "install", "--install-dir", dir.shellescape, "pkg/#{filename.shellescape}", exception: true
        assert_path_exist File.join(dir, "gems/whispercpp-1.3.0/lib", basename)
      end
    end
  end
end
