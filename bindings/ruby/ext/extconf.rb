require 'mkmf'

# need to use c++ compiler flags
$CXXFLAGS << ' -std=c++11'

$LDFLAGS << ' -lstdc++'

# Set to true when building binary gems
if enable_config('static-stdlib', false)
  $LDFLAGS << ' -static-libgcc -static-libstdc++'
end

if enable_config('march-tune-native', false)
  $CFLAGS << ' -march=native -mtune=native'
  $CXXFLAGS << ' -march=native -mtune=native'
end

if ENV['WHISPER_METAL']
  $GGML_METAL ||= true
  $DEPRECATE_WARNING ||= true
end

$UNAME_S = `uname -s`.chomp
$UNAME_P = `uname -p`.chomp
$UNAME_M = `uname -m`.chomp

if $UNAME_S == 'Darwin'
  unless ENV['GGML_NO_METAL']
    $GGML_METAL ||= true
  end
  $GGML_NO_OPENMP ||= true
end

if $GGML_METAL
  $GGML_METAL_EMBED_LIBRARY = true
end

$MK_CPPFLAGS = ''
$MK_CFLAGS   = '-std=c11   -fPIC'
$MK_CXXFLAGS = '-std=c++11 -fPIC'
$MK_NVCCFLAGS = '-std=c++11'
$MK_LDFLAGS = ''

$OBJ_GGML = []
$OBJ_WHISPER = []
$OBJ_COMMON = []
$OBJ_SDL = []

$MK_CPPFLAGS << ' -D_XOPEN_SOURCE=600'

if $UNAME_S == 'Linux'
  $MK_CPPFLAGS << ' -D_GNU_SOURCE'
end

if $UNAME_S == 'Darwin'
  $MK_CPPFLAGS << ' -D_DARWIN_C_SOURCE'
end

if ENV['WHISPER_DEBUG']
  $MK_CFLAGS    << ' -O0 -g'
  $MK_CXXFLAGS  << ' -O0 -g'
  $MK_LDFLAGS   << ' -g'
  $MK_NVCCFLAGS << ' -O0 -g'
else
  $MK_CPPFLAGS   << ' -DNDEBUG'
  $MK_CFLAGS     << ' -O3'
  $MK_CXXFLAGS   << ' -O3'
  $MK_NVCCFLAGS  << ' -O3'
end

$WARN_FLAGS =
  ' -Wall' <<
  ' -Wextra' <<
  ' -Wpedantic' <<
  ' -Wcast-qual' <<
  ' -Wno-unused-function'

$MK_CFLAGS <<
  $WARN_FLAGS <<
  ' -Wshadow' <<
  ' -Wstrict-prototypes' <<
  ' -Wpointer-arith' <<
  ' -Wmissing-prototypes' <<
  ' -Werror=implicit-int' <<
  ' -Werror=implicit-function-declaration'

$MK_CXXFLAGS <<
  $WARN_FLAGS <<
  ' -Wmissing-declarations' <<
  ' -Wmissing-noreturn'

unless `#{cc_command} #{$LDFLAGS} -Wl,-v 2>&1`.chomp.include? 'dyld-1015.7'
  $MK_CPPFLAGS << ' -DHAVE_BUGGY_APPLE_LINKER'
end

if %w[Linux Darwin FreeBSD NetBSD OpenBSD Haiku].include? $UNAME_S
  $MK_CFLAGS   << ' -pthread'
  $MK_CXXFLAGS << ' -pthread'
end

unless $_WIN32
  $DSO_EXT = '.so'
else
  $DSO_EXT = '.dll'
end

unless ENV['RISCV']
  if %w[x86_64 i686 amd64].include? $UNAME_M
    $HOST_CXXFLAGS ||= ''

    $MK_CFLAGS     << ' -march=native -mtune=native'
    $HOST_CXXFLAGS << ' -march=native -mtune=native'
  end

  if $UNAME_M.match? /aarch64.*/
    $MK_CFLAGS   << ' -mcpu=native'
    $MK_CXXFLAGS << ' -mcpu=native'
  end
else
  $MK_CFLAGS   << ' -march=rv64gcv -mabi=lp64d'
  $MK_CXXFLAGS << ' -march=rv64gcv -mabi=lp64d'
end

unless ENV['GGML_NO_ACCELERATE']
  if $UNAME_S == 'Darwin'
    $MK_CPPFLAGS << ' -DGGML_USE_ACCELERATE -DGGML_USE_BLAS'
    $MK_CPPFLAGS << ' -DACCELERATE_NEW_LAPACK'
    $MK_CPPFLAGS << ' -DACCELERATE_LAPACK_ILP64'
    $MK_LDFLAGS  << ' -framework Accelerate'
    $OBJ_GGML    << 'ggml-blas.o'
  end
end

if ENV['GGML_OPENBLAS']
  $MK_CPPFLAGS << " -DGGML_USE_BLAS #{`pkg-config --cflags-only-I openblas`.chomp}"
  $MK_CFLAGS   << " #{`pkg-config --cflags-only-other openblas)`.chomp}"
  $MK_LDFLAGS  << " #{`pkg-config --libs openblas`}"
  $OBJ_GGML    << 'ggml-blas.o'
end

if ENV['GGML_OPENBLAS64']
  $MK_CPPFLAGS << " -DGGML_USE_BLAS #{`pkg-config --cflags-only-I openblas64`.chomp}"
  $MK_CFLAGS   << " #{`pkg-config --cflags-only-other openblas64)`.chomp}"
  $MK_LDFLAGS  << " #{`pkg-config --libs openblas64`}"
  $OBJ_GGML    << 'ggml-blas.o'
end

if $GGML_METAL
  $MK_CPPFLAGS << ' -DGGML_USE_METAL'
  $MK_LDFLAGS  << ' -framework Foundation -framework Metal -framework MetalKit'
  $OBJ_GGML    << 'ggml-metal.o'

  if ENV['GGML_METAL_NDEBUG']
    $MK_CPPFLAGS << ' -DGGML_METAL_NDEBUG'
  end

  if $GGML_METAL_EMBED_LIBRARY
    $MK_CPPFLAGS << ' -DGGML_METAL_EMBED_LIBRARY'
    $OBJ_GGML    << 'ggml-metal-embed.o'
  end
end

$OBJ_GGML <<
  'ggml.o' <<
  'ggml-alloc.o' <<
  'ggml-backend.o' <<
  'ggml-quants.o' <<
  'ggml-aarch64.o'

$OBJ_WHISPER <<
  'whisper.o'

$objs = $OBJ_GGML + $OBJ_WHISPER + $OBJ_COMMON + $OBJ_SDL
$objs << "ruby_whisper.o"

$CPPFLAGS  = "#{$MK_CPPFLAGS} #{$CPPFLAGS}"
$CFLAGS    = "#{$CPPFLAGS} #{$MK_CFLAGS} #{$GF_CFLAGS} #{$CFLAGS}"
$BASE_CXXFLAGS = "#{$MK_CXXFLAGS} #{$CXXFLAGS}"
$CXXFLAGS  = "#{$BASE_CXXFLAGS} #{$HOST_CXXFLAGS} #{$GF_CXXFLAGS} #{$CPPFLAGS}"
$NVCCFLAGS = "#{$MK_NVCCFLAGS} #{$NVCCFLAGS}"
$LDFLAGS   = "#{$MK_LDFLAGS} #{$LDFLAGS}"

create_makefile('whisper')

File.open 'Makefile', 'a' do |file|
  file.puts 'include get-flags.mk'

  if $GGML_METAL
    if $GGML_METAL_EMBED_LIBRARY
      file.puts 'include metal-embed.mk'
    end
  end
end
