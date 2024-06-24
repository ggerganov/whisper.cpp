Gem::Specification.new do |s|
  s.name    = "whispercpp"
  s.authors = ["Georgi Gerganov", "Todd A. Fisher"]
  s.version = '1.3.0'
  s.date    = '2024-05-14'
  s.description = %q{High-performance inference of OpenAI's Whisper automatic speech recognition (ASR) model via Ruby}
  s.email   = 'todd.fisher@gmail.com'
  s.extra_rdoc_files = ['LICENSE', 'README.md']
  
  s.files = ["LICENSE", "README.md", "Rakefile", "ext/extconf.rb", "ext/ggml.c", "ext/ruby_whisper.cpp", "ext/whisper.cpp", "ext/dr_wav.h", "ext/ggml.h", "ext/ruby_whisper.h", "ext/whisper.h"]

  #### Load-time details
  s.require_paths = ['lib','ext']
  s.summary = %q{Ruby whisper.cpp bindings}
  s.test_files = ["tests/test_whisper.rb"]
  
  s.extensions << 'ext/extconf.rb'
  

  #### Documentation and testing.
  s.homepage = 'https://github.com/ggerganov/whisper.cpp'
  s.rdoc_options = ['--main', '../../README.md']

  
    s.platform = Gem::Platform::RUBY
  
  s.licenses = ['MIT']
end
