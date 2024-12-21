require_relative "helper"
require 'tempfile'
require 'tmpdir'
require 'shellwords'

class TestPackage < TestBase
  def test_build
    Tempfile.create do |file|
      assert system("gem", "build", "whispercpp.gemspec", "--output", file.to_path.shellescape, exception: true)
      assert file.size > 0
      assert_path_exist file.to_path
    end
  end

  sub_test_case "Building binary on installation" do
    def setup
      system "rake", "build", exception: true
    end

    def test_install
      match_data = `rake -Tbuild`.match(/(whispercpp-(.+)\.gem)/)
      filename = match_data[1]
      version = match_data[2]
      basename = "whisper.#{RbConfig::CONFIG["DLEXT"]}"
      Dir.mktmpdir do |dir|
        system "gem", "install", "--install-dir", dir.shellescape, "--no-document", "pkg/#{filename.shellescape}", exception: true
        assert_path_exist File.join(dir, "gems/whispercpp-#{version}/lib", basename)
      end
    end
  end
end
