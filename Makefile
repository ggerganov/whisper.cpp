default: main bench quantize

ifndef UNAME_S
UNAME_S := $(shell uname -s)
endif

ifndef UNAME_P
UNAME_P := $(shell uname -p)
endif

ifndef UNAME_M
UNAME_M := $(shell uname -m)
endif

ifndef NVCC_VERSION
	ifeq ($(call,$(shell which nvcc))$(.SHELLSTATUS),0)
		NVCC_VERSION := $(shell nvcc --version | egrep -o "V[0-9]+.[0-9]+.[0-9]+" | cut -c2-)
	endif
endif

CCV := $(shell $(CC) --version | head -n 1)
CXXV := $(shell $(CXX) --version | head -n 1)

# Mac OS + Arm can report x86_64
# ref: https://github.com/ggerganov/whisper.cpp/issues/66#issuecomment-1282546789
ifeq ($(UNAME_S),Darwin)
	ifneq ($(UNAME_P),arm)
		SYSCTL_M := $(shell sysctl -n hw.optional.arm64)
		ifeq ($(SYSCTL_M),1)
			# UNAME_P := arm
			# UNAME_M := arm64
			warn := $(warning Your arch is announced as x86_64, but it seems to actually be ARM64. Not fixing that can lead to bad performance. For more info see: https://github.com/ggerganov/whisper.cpp/issues/66\#issuecomment-1282546789)
		endif
	endif
endif

#
# Compile flags
#

CFLAGS   = -I.              -O3 -DNDEBUG -std=c11   -fPIC
CXXFLAGS = -I. -I./examples -O3 -DNDEBUG -std=c++11 -fPIC
LDFLAGS  =

# ref: https://github.com/ggerganov/whisper.cpp/issues/37
ifneq ($(wildcard /usr/include/musl/*),)
	CFLAGS += -D_POSIX_SOURCE -D_GNU_SOURCE
	CXXFLAGS += -D_POSIX_SOURCE -D_GNU_SOURCE
endif

# RLIMIT_MEMLOCK came in BSD, is not specified in POSIX.1,
# and on macOS its availability depends on enabling Darwin extensions
ifeq ($(UNAME_S),Darwin)
	CFLAGS   += -D_DARWIN_C_SOURCE
	CXXFLAGS += -D_DARWIN_C_SOURCE
endif

# OS specific
# TODO: support Windows
ifeq ($(filter $(UNAME_S),Linux Darwin DragonFly FreeBSD NetBSD OpenBSD Haiku),$(UNAME_S))
	CFLAGS   += -pthread
	CXXFLAGS += -pthread
endif

# Architecture specific
# TODO: probably these flags need to be tweaked on some architectures
#       feel free to update the Makefile for your architecture and send a pull request or issue
ifeq ($(UNAME_M),$(filter $(UNAME_M),x86_64 i686))
	ifeq ($(UNAME_S),Darwin)
		CPUINFO_CMD := sysctl machdep.cpu.features
	else ifeq ($(UNAME_S),Linux)
		CPUINFO_CMD := cat /proc/cpuinfo
	else ifneq (,$(filter MINGW32_NT% MINGW64_NT%,$(UNAME_S)))
		CPUINFO_CMD := cat /proc/cpuinfo
	else ifeq ($(UNAME_S),Haiku)
		CPUINFO_CMD := sysinfo -cpu
	endif

	ifdef CPUINFO_CMD  	
    AVX_M := $(shell $(CPUINFO_CMD) | grep -m 1 "avx ")
		ifneq (,$(findstring avx,$(AVX_M)))
			CFLAGS += -mavx
		endif
    
		AVX2_M := $(shell $(CPUINFO_CMD) | grep -m 1 "avx2 ")
		ifneq (,$(findstring avx2,$(AVX2_M)))
			CFLAGS += -mavx2
		endif

		FMA_M := $(shell $(CPUINFO_CMD) | grep -m 1 "fma ")
		ifneq (,$(findstring fma,$(FMA_M)))
			CFLAGS += -mfma
		endif

		F16C_M := $(shell $(CPUINFO_CMD) | grep -m 1 "f16c ")
		ifneq (,$(findstring f16c,$(F16C_M)))
			CFLAGS += -mf16c

			AVX1_M := $(shell $(CPUINFO_CMD) | grep -m 1 "avx ")
			ifneq (,$(findstring avx,$(AVX1_M)))
				CFLAGS += -mavx
			endif
		endif

		SSE3_M := $(shell $(CPUINFO_CMD) | grep -m 1 "sse3 ")
		ifneq (,$(findstring sse3,$(SSE3_M)))
			CFLAGS += -msse3
		endif

		SSSE3_M := $(shell $(CPUINFO_CMD) | grep -m 1 "ssse3 ")
		ifneq (,$(findstring ssse3,$(SSSE3_M)))
			CFLAGS += -mssse3
		endif
	endif
endif
ifeq ($(UNAME_M),amd64)
	CFLAGS += -mavx -mavx2 -mfma -mf16c
endif

ifneq ($(filter ppc64%,$(UNAME_M)),)
	POWER9_M := $(shell grep "POWER9" /proc/cpuinfo)
	ifneq (,$(findstring POWER9,$(POWER9_M)))
		CFLAGS += -mpower9-vector
	endif
	# Require c++23's std::byteswap for big-endian support.
	ifeq ($(UNAME_M),ppc64)
		CXXFLAGS += -std=c++23 -DGGML_BIG_ENDIAN
	endif
endif

ifndef WHISPER_NO_ACCELERATE
	# Mac M1 - include Accelerate framework
	ifeq ($(UNAME_S),Darwin)
		CFLAGS  += -DGGML_USE_ACCELERATE
		LDFLAGS += -framework Accelerate
	endif
endif

ifdef WHISPER_COREML
	CXXFLAGS += -DWHISPER_USE_COREML
	LDFLAGS  += -framework Foundation -framework CoreML

ifdef WHISPER_COREML_ALLOW_FALLBACK
	CXXFLAGS += -DWHISPER_COREML_ALLOW_FALLBACK
endif
endif

ifdef WHISPER_OPENBLAS
	CFLAGS  += -DGGML_USE_OPENBLAS -I/usr/local/include/openblas -I/usr/include/openblas
	LDFLAGS += -lopenblas
endif

ifdef WHISPER_CUBLAS
	ifeq ($(shell expr $(NVCC_VERSION) \>= 11.6), 1)
		CUDA_ARCH_FLAG=native
	else
		CUDA_ARCH_FLAG=all
	endif

	CFLAGS      += -DGGML_USE_CUBLAS -I/usr/local/cuda/include -I/opt/cuda/include -I$(CUDA_PATH)/targets/$(UNAME_M)-linux/include
	CXXFLAGS    += -DGGML_USE_CUBLAS -I/usr/local/cuda/include -I/opt/cuda/include -I$(CUDA_PATH)/targets/$(UNAME_M)-linux/include
	LDFLAGS     += -lcublas -lculibos -lcudart -lcublasLt -lpthread -ldl -lrt -L/usr/local/cuda/lib64 -L/opt/cuda/lib64 -L$(CUDA_PATH)/targets/$(UNAME_M)-linux/lib
	WHISPER_OBJ += ggml-cuda.o
	NVCC        = nvcc
	NVCCFLAGS   = --forward-unknown-to-host-compiler -arch=$(CUDA_ARCH_FLAG)

ggml-cuda.o: ggml-cuda.cu ggml-cuda.h
	$(NVCC) $(NVCCFLAGS) $(CXXFLAGS) -Wno-pedantic -c $< -o $@
endif

ifdef WHISPER_HIPBLAS
	ROCM_PATH   ?= /opt/rocm
	HIPCC       ?= $(ROCM_PATH)/bin/hipcc
	GPU_TARGETS ?= $(shell $(ROCM_PATH)/llvm/bin/amdgpu-arch)
	CFLAGS      += -DGGML_USE_HIPBLAS -DGGML_USE_CUBLAS
	CXXFLAGS    += -DGGML_USE_HIPBLAS -DGGML_USE_CUBLAS
	LDFLAGS     += -L$(ROCM_PATH)/lib -Wl,-rpath=$(ROCM_PATH)/lib
	LDFLAGS     += -lhipblas -lamdhip64 -lrocblas
	HIPFLAGS    += $(addprefix --offload-arch=,$(GPU_TARGETS))
	WHISPER_OBJ += ggml-cuda.o

ggml-cuda.o: ggml-cuda.cu ggml-cuda.h
	$(HIPCC) $(CXXFLAGS) $(HIPFLAGS) -x hip -c -o $@ $<
endif

ifdef WHISPER_CLBLAST
	CFLAGS 		+= -DGGML_USE_CLBLAST
	CXXFLAGS 	+= -DGGML_USE_CLBLAST
	LDFLAGS	 	+= -lclblast
	ifeq ($(UNAME_S),Darwin)
		LDFLAGS	 	+= -framework OpenCL
	else
		LDFLAGS	    += -lOpenCL
	endif
	WHISPER_OBJ	+= ggml-opencl.o

ggml-opencl.o: ggml-opencl.cpp ggml-opencl.h
	$(CXX) $(CXXFLAGS) -c $< -o $@
endif

ifdef WHISPER_GPROF
	CFLAGS   += -pg
	CXXFLAGS += -pg
endif

ifneq ($(filter aarch64%,$(UNAME_M)),)
	CFLAGS   += -mcpu=native
	CXXFLAGS += -mcpu=native
endif

ifneq ($(filter armv6%,$(UNAME_M)),)
	# 32-bit Raspberry Pi 1, 2, 3
	CFLAGS += -mfpu=neon -mfp16-format=ieee -mno-unaligned-access
endif

ifneq ($(filter armv7%,$(UNAME_M)),)
	# 32-bit ARM, for example on Armbian or possibly raspbian
	#CFLAGS   += -mfpu=neon -mfp16-format=ieee -funsafe-math-optimizations -mno-unaligned-access
	#CXXFLAGS += -mfpu=neon -mfp16-format=ieee -funsafe-math-optimizations -mno-unaligned-access

	# 64-bit ARM on 32-bit OS, use these (TODO: auto-detect 64-bit)
	CFLAGS   += -mfpu=neon-fp-armv8 -mfp16-format=ieee -funsafe-math-optimizations -mno-unaligned-access
	CXXFLAGS += -mfpu=neon-fp-armv8 -mfp16-format=ieee -funsafe-math-optimizations -mno-unaligned-access
endif

ifneq ($(filter armv8%,$(UNAME_M)),)
	# Raspberry Pi 4
	CFLAGS   += -mfpu=neon-fp-armv8 -mfp16-format=ieee -funsafe-math-optimizations -mno-unaligned-access
	CXXFLAGS += -mfpu=neon-fp-armv8 -mfp16-format=ieee -funsafe-math-optimizations -mno-unaligned-access
endif

#
# Print build information
#

$(info I whisper.cpp build info: )
$(info I UNAME_S:  $(UNAME_S))
$(info I UNAME_P:  $(UNAME_P))
$(info I UNAME_M:  $(UNAME_M))
$(info I CFLAGS:   $(CFLAGS))
$(info I CXXFLAGS: $(CXXFLAGS))
$(info I LDFLAGS:  $(LDFLAGS))
$(info I CC:       $(CCV))
$(info I CXX:      $(CXXV))
$(info )

#
# Build library
#

ggml.o: ggml.c ggml.h ggml-cuda.h
	$(CC)  $(CFLAGS)   -c $< -o $@

whisper.o: whisper.cpp whisper.h ggml.h ggml-cuda.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

ifndef WHISPER_COREML
WHISPER_OBJ += whisper.o
else
whisper-encoder.o: coreml/whisper-encoder.mm coreml/whisper-encoder.h
	$(CXX) -O3 -I . -fobjc-arc -c coreml/whisper-encoder.mm -o whisper-encoder.o

whisper-encoder-impl.o: coreml/whisper-encoder-impl.m coreml/whisper-encoder-impl.h
	$(CXX) -O3 -I . -fobjc-arc -c coreml/whisper-encoder-impl.m -o whisper-encoder-impl.o

WHISPER_OBJ += whisper.o whisper-encoder.o whisper-encoder-impl.o
endif

libwhisper.a: ggml.o $(WHISPER_OBJ)
	$(AR) rcs libwhisper.a ggml.o $(WHISPER_OBJ)

libwhisper.so: ggml.o $(WHISPER_OBJ)
	$(CXX) $(CXXFLAGS) -shared -o libwhisper.so ggml.o $(WHISPER_OBJ) $(LDFLAGS)

clean:
	rm -f *.o main stream command talk talk-llama bench quantize lsp libwhisper.a libwhisper.so

#
# Examples
#

CC_SDL=`sdl2-config --cflags --libs`

SRC_COMMON     = examples/common.cpp examples/common-ggml.cpp
SRC_COMMON_SDL = examples/common-sdl.cpp

main: examples/main/main.cpp $(SRC_COMMON) ggml.o $(WHISPER_OBJ)
	$(CXX) $(CXXFLAGS) examples/main/main.cpp $(SRC_COMMON) ggml.o $(WHISPER_OBJ) -o main $(LDFLAGS)
	./main -h

bench: examples/bench/bench.cpp ggml.o $(WHISPER_OBJ)
	$(CXX) $(CXXFLAGS) examples/bench/bench.cpp ggml.o $(WHISPER_OBJ) -o bench $(LDFLAGS)

quantize: examples/quantize/quantize.cpp ggml.o $(WHISPER_OBJ) $(SRC_COMMON)
	$(CXX) $(CXXFLAGS) examples/quantize/quantize.cpp $(SRC_COMMON) ggml.o $(WHISPER_OBJ) -o quantize $(LDFLAGS)

stream: examples/stream/stream.cpp $(SRC_COMMON) $(SRC_COMMON_SDL) ggml.o $(WHISPER_OBJ)
	$(CXX) $(CXXFLAGS) examples/stream/stream.cpp $(SRC_COMMON) $(SRC_COMMON_SDL) ggml.o $(WHISPER_OBJ) -o stream $(CC_SDL) $(LDFLAGS)

command: examples/command/command.cpp $(SRC_COMMON) $(SRC_COMMON_SDL) ggml.o $(WHISPER_OBJ)
	$(CXX) $(CXXFLAGS) examples/command/command.cpp $(SRC_COMMON) $(SRC_COMMON_SDL) ggml.o $(WHISPER_OBJ) -o command $(CC_SDL) $(LDFLAGS)

lsp: examples/lsp/lsp.cpp $(SRC_COMMON) $(SRC_COMMON_SDL) ggml.o $(WHISPER_OBJ)
	$(CXX) $(CXXFLAGS) examples/lsp/lsp.cpp $(SRC_COMMON) $(SRC_COMMON_SDL) ggml.o $(WHISPER_OBJ) -o lsp $(CC_SDL) $(LDFLAGS)

talk: examples/talk/talk.cpp examples/talk/gpt-2.cpp $(SRC_COMMON) $(SRC_COMMON_SDL) ggml.o $(WHISPER_OBJ)
	$(CXX) $(CXXFLAGS) examples/talk/talk.cpp examples/talk/gpt-2.cpp $(SRC_COMMON) $(SRC_COMMON_SDL) ggml.o $(WHISPER_OBJ) -o talk $(CC_SDL) $(LDFLAGS)

talk-llama: examples/talk-llama/talk-llama.cpp examples/talk-llama/llama.cpp $(SRC_COMMON) $(SRC_COMMON_SDL) ggml.o $(WHISPER_OBJ)
	$(CXX) $(CXXFLAGS) examples/talk-llama/talk-llama.cpp examples/talk-llama/llama.cpp $(SRC_COMMON) $(SRC_COMMON_SDL) ggml.o $(WHISPER_OBJ) -o talk-llama $(CC_SDL) $(LDFLAGS)

#
# Audio samples
#

# download a few audio samples into folder "./samples":
.PHONY: samples
samples:
	@echo "Downloading samples..."
	@mkdir -p samples
	@wget --quiet --show-progress -O samples/gb0.ogg https://upload.wikimedia.org/wikipedia/commons/2/22/George_W._Bush%27s_weekly_radio_address_%28November_1%2C_2008%29.oga
	@wget --quiet --show-progress -O samples/gb1.ogg https://upload.wikimedia.org/wikipedia/commons/1/1f/George_W_Bush_Columbia_FINAL.ogg
	@wget --quiet --show-progress -O samples/hp0.ogg https://upload.wikimedia.org/wikipedia/en/d/d4/En.henryfphillips.ogg
	@wget --quiet --show-progress -O samples/mm1.wav https://cdn.openai.com/whisper/draft-20220913a/micro-machines.wav
	@wget --quiet --show-progress -O samples/a13.mp3 https://upload.wikimedia.org/wikipedia/commons/transcoded/6/6f/Apollo13-wehaveaproblem.ogg/Apollo13-wehaveaproblem.ogg.mp3
	@wget --quiet --show-progress -O samples/diffusion2023-07-03.flac https://archive.org/download/diffusion2023-07-03/diffusion2023-07-03.flac
	@echo "Converting to 16-bit WAV ..."
	@ffmpeg -loglevel -0 -y -i samples/gb0.ogg -ar 16000 -ac 1 -c:a pcm_s16le samples/gb0.wav
	@ffmpeg -loglevel -0 -y -i samples/gb1.ogg -ar 16000 -ac 1 -c:a pcm_s16le samples/gb1.wav
	@ffmpeg -loglevel -0 -y -i samples/hp0.ogg -ar 16000 -ac 1 -c:a pcm_s16le samples/hp0.wav
	@rm samples/*.ogg
	@ffmpeg -loglevel -0 -y -i samples/mm1.wav -ar 16000 -ac 1 -c:a pcm_s16le samples/mm0.wav
	@rm samples/mm1.wav
	@ffmpeg -loglevel -0 -y -i samples/a13.mp3 -ar 16000 -ac 1 -c:a pcm_s16le -ss 00:00:00 -to 00:00:30 samples/a13.wav
	@rm samples/a13.mp3
	@ffmpeg -loglevel -0 -y -i samples/diffusion2023-07-03.flac -ar 16000 -ac 1 -c:a pcm_s16le samples/diffusion2023-07-03.wav
	@rm samples/diffusion2023-07-03.flac

#
# Models
#

# if not already downloaded, the following targets download the specified model and
# runs it on all samples in the folder "./samples":

.PHONY: tiny.en
.PHONY: tiny
.PHONY: base.en
.PHONY: base
.PHONY: small.en
.PHONY: small
.PHONY: medium.en
.PHONY: medium
.PHONY: large-v1
.PHONY: large

tiny.en tiny base.en base small.en small medium.en medium large-v1 large: main
	bash ./models/download-ggml-model.sh $@
	@echo ""
	@echo "==============================================="
	@echo "Running $@ on all samples in ./samples ..."
	@echo "==============================================="
	@echo ""
	@for f in samples/*.wav; do \
		echo "----------------------------------------------" ; \
		echo "[+] Running $@ on $$f ... (run 'ffplay $$f' to listen)" ; \
	    echo "----------------------------------------------" ; \
		echo "" ; \
		./main -m models/ggml-$@.bin -f $$f ; \
		echo "" ; \
	done

#
# Tests
#

.PHONY: tests
tests:
	bash ./tests/run-tests.sh $(word 2, $(MAKECMDGOALS))
