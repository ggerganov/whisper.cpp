require 'mkmf'
system("cp #{File.join(File.dirname(__FILE__),'..','..','..','whisper.cpp')} .")
system("cp #{File.join(File.dirname(__FILE__),'..','..','..','whisper.h')} .")
system("cp #{File.join(File.dirname(__FILE__),'..','..','..','ggml.h')} .")
system("cp #{File.join(File.dirname(__FILE__),'..','..','..','ggml.c')} .")
system("cp #{File.join(File.dirname(__FILE__),'..','..','..','ggml-impl.h')} .")
system("cp #{File.join(File.dirname(__FILE__),'..','..','..','ggml-aarch64.h')} .")
system("cp #{File.join(File.dirname(__FILE__),'..','..','..','ggml-aarch64.c')} .")
system("cp #{File.join(File.dirname(__FILE__),'..','..','..','ggml-alloc.h')} .")
system("cp #{File.join(File.dirname(__FILE__),'..','..','..','ggml-alloc.c')} .")
system("cp #{File.join(File.dirname(__FILE__),'..','..','..','ggml-backend-impl.h')} .")
system("cp #{File.join(File.dirname(__FILE__),'..','..','..','ggml-backend.h')} .")
system("cp #{File.join(File.dirname(__FILE__),'..','..','..','ggml-backend.cpp')} .")
system("cp #{File.join(File.dirname(__FILE__),'..','..','..','ggml-common.h')} .")
system("cp #{File.join(File.dirname(__FILE__),'..','..','..','ggml-quants.h')} .")
system("cp #{File.join(File.dirname(__FILE__),'..','..','..','ggml-quants.c')} .")
system("cp #{File.join(File.dirname(__FILE__),'..','..','..','examples','dr_wav.h')} .")


# need to use c++ compiler flags
$CXXFLAGS << ' -std=c++11'
# Set to true when building binary gems
if enable_config('static-stdlib', false)
  $LDFLAGS << ' -static-libgcc -static-libstdc++'
end

if enable_config('march-tune-native', false)
  $CFLAGS << ' -march=native -mtune=native'
  $CXXFLAGS << ' -march=native -mtune=native'
end

create_makefile('whisper')
