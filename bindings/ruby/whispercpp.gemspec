require_relative "extsources"

Gem::Specification.new do |s|
  s.name    = "whispercpp"
  s.authors = ["Georgi Gerganov", "Todd A. Fisher"]
  s.version = '1.3.0'
  s.date    = '2024-05-14'
  s.description = %q{High-performance inference of OpenAI's Whisper automatic speech recognition (ASR) model via Ruby}
  s.email   = 'todd.fisher@gmail.com'
  s.extra_rdoc_files = ['LICENSE', 'README.md']
  
  s.files = `git ls-files . -z`.split("\x0") +
              EXTSOURCES.collect {|file|
                basename = File.basename(file)
                if s.extra_rdoc_files.include?(basename)
                  basename
                else
                  file.sub("../..", "ext")
                end
              }

  s.summary = %q{Ruby whisper.cpp bindings}
  s.test_files = s.files.select {|file| file.start_with? "tests/"}
  
  s.extensions << 'ext/extconf.rb'
  

  #### Documentation and testing.
  s.homepage = 'https://github.com/ggerganov/whisper.cpp'
  s.rdoc_options = ['--main', 'README.md']

  
    s.platform = Gem::Platform::RUBY
  
  s.licenses = ['MIT']
end
