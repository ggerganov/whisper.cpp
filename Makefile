default: main bench quantize server

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

CCV  := $(shell $(CC) --version | head -n 1)
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

ifdef MACOSX_DEPLOYMENT_TARGET
	CFLAGS   += -mmacosx-version-min=$(MACOSX_DEPLOYMENT_TARGET)
	CXXFLAGS += -mmacosx-version-min=$(MACOSX_DEPLOYMENT_TARGET)
	LDFLAGS  += -mmacosx-version-min=$(MACOSX_DEPLOYMENT_TARGET)
endif

# clock_gettime came in POSIX.1b (1993)
# CLOCK_MONOTONIC came in POSIX.1-2001 / SUSv3 as optional
# posix_memalign came in POSIX.1-2001 / SUSv3
# M_PI is an XSI extension since POSIX.1-2001 / SUSv3, came in XPG1 (1985)
CFLAGS   += -D_XOPEN_SOURCE=600
CXXFLAGS += -D_XOPEN_SOURCE=600

# Somehow in OpenBSD whenever POSIX conformance is specified
# some string functions rely on locale_t availability,
# which was introduced in POSIX.1-2008, forcing us to go higher
ifeq ($(UNAME_S),OpenBSD)
	CFLAGS   += -U_XOPEN_SOURCE -D_XOPEN_SOURCE=700
	CXXFLAGS += -U_XOPEN_SOURCE -D_XOPEN_SOURCE=700
endif

# Data types, macros and functions related to controlling CPU affinity
# are available on Linux through GNU extensions in libc
ifeq ($(UNAME_S),Linux)
	CFLAGS   += -D_GNU_SOURCE
	CXXFLAGS += -D_GNU_SOURCE
endif

# RLIMIT_MEMLOCK came in BSD, is not specified in POSIX.1,
# and on macOS its availability depends on enabling Darwin extensions
# similarly on DragonFly, enabling BSD extensions is necessary
ifeq ($(UNAME_S),Darwin)
	CFLAGS   += -D_DARWIN_C_SOURCE
	CXXFLAGS += -D_DARWIN_C_SOURCE
endif
ifeq ($(UNAME_S),DragonFly)
	CFLAGS   += -D__BSD_VISIBLE
	CXXFLAGS += -D__BSD_VISIBLE
endif

# alloca is a non-standard interface that is not visible on BSDs when
# POSIX conformance is specified, but not all of them provide a clean way
# to enable it in such cases
ifeq ($(UNAME_S),FreeBSD)
	CFLAGS   += -D__BSD_VISIBLE
	CXXFLAGS += -D__BSD_VISIBLE
endif
ifeq ($(UNAME_S),NetBSD)
	CFLAGS   += -D_NETBSD_SOURCE
	CXXFLAGS += -D_NETBSD_SOURCE
endif
ifeq ($(UNAME_S),OpenBSD)
	CFLAGS   += -D_BSD_SOURCE
	CXXFLAGS += -D_BSD_SOURCE
endif

# OS specific
# TODO: support Windows
ifeq ($(filter $(UNAME_S),Linux Darwin DragonFly FreeBSD NetBSD OpenBSD Haiku),$(UNAME_S))
	CFLAGS   += -pthread
	CXXFLAGS += -pthread
endif

# detect Windows
ifneq ($(findstring _NT,$(UNAME_S)),)
	_WIN32 := 1
endif

# Windows Sockets 2 (Winsock) for network-capable apps
ifeq ($(_WIN32),1)
	LWINSOCK2 := -lws2_32
endif

# Architecture specific
# TODO: probably these flags need to be tweaked on some architectures
#       feel free to update the Makefile for your architecture and send a pull request or issue
ifeq ($(UNAME_M),$(filter $(UNAME_M),x86_64 i686 amd64))
	ifeq ($(UNAME_S),Darwin)
		CPUINFO_CMD := sysctl machdep.cpu.features machdep.cpu.leaf7_features
	else ifeq ($(UNAME_S),Linux)
		CPUINFO_CMD := cat /proc/cpuinfo
	else ifneq (,$(filter MINGW32_NT% MINGW64_NT% MSYS_NT%,$(UNAME_S)))
		CPUINFO_CMD := cat /proc/cpuinfo
	else ifneq (,$(filter DragonFly FreeBSD,$(UNAME_S)))
		CPUINFO_CMD := grep Features /var/run/dmesg.boot
	else ifeq ($(UNAME_S),Haiku)
		CPUINFO_CMD := sysinfo -cpu
	endif

	ifdef CPUINFO_CMD
		AVX_M := $(shell $(CPUINFO_CMD) | grep -iwE 'AVX|AVX1.0')
		ifneq (,$(AVX_M))
			CFLAGS   += -mavx
			CXXFLAGS += -mavx
		endif

		AVX2_M := $(shell $(CPUINFO_CMD) | grep -iw 'AVX2')
		ifneq (,$(AVX2_M))
			CFLAGS   += -mavx2
			CXXFLAGS += -mavx2
		endif

		FMA_M := $(shell $(CPUINFO_CMD) | grep -iw 'FMA')
		ifneq (,$(FMA_M))
			CFLAGS   += -mfma
			CXXFLAGS += -mfma
		endif

		F16C_M := $(shell $(CPUINFO_CMD) | grep -iw 'F16C')
		ifneq (,$(F16C_M))
			CFLAGS   += -mf16c
			CXXFLAGS += -mf16c
		endif

		SSE3_M := $(shell $(CPUINFO_CMD) | grep -iwE 'PNI|SSE3')
		ifneq (,$(SSE3_M))
			CFLAGS   += -msse3
			CXXFLAGS += -msse3
		endif

		SSSE3_M := $(shell $(CPUINFO_CMD) | grep -iw 'SSSE3')
		ifneq (,$(SSSE3_M))
			CFLAGS   += -mssse3
			CXXFLAGS += -mssse3
		endif
	endif
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
		CFLAGS  += -DACCELERATE_NEW_LAPACK
		CFLAGS  += -DACCELERATE_LAPACK_ILP64
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

ifndef WHISPER_NO_METAL
	ifeq ($(UNAME_S),Darwin)
		WHISPER_METAL := 1

		CFLAGS   += -DGGML_USE_METAL
		CXXFLAGS += -DGGML_USE_METAL
		LDFLAGS  += -framework Foundation -framework Metal -framework MetalKit
	endif
endif

ifdef WHISPER_OPENBLAS
	CFLAGS  += -DGGML_USE_OPENBLAS -I/usr/local/include/openblas -I/usr/include/openblas
	LDFLAGS += -lopenblas
endif

ifdef WHISPER_CUBLAS
	ifeq ($(shell expr $(NVCC_VERSION) \>= 11.6), 1)
		CUDA_ARCH_FLAG ?= native
	else
		CUDA_ARCH_FLAG ?= all
	endif

	CFLAGS      += -DGGML_USE_CUBLAS -I/usr/local/cuda/include -I/opt/cuda/include -I$(CUDA_PATH)/targets/$(UNAME_M)-linux/include
	CXXFLAGS    += -DGGML_USE_CUBLAS -I/usr/local/cuda/include -I/opt/cuda/include -I$(CUDA_PATH)/targets/$(UNAME_M)-linux/include
	LDFLAGS     += -lcuda -lcublas -lculibos -lcudart -lcublasLt -lpthread -ldl -lrt -L/usr/local/cuda/lib64 -L/opt/cuda/lib64 -L$(CUDA_PATH)/targets/$(UNAME_M)-linux/lib -L/usr/lib/wsl/lib
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

ggml-alloc.o: ggml-alloc.c ggml.h ggml-alloc.h
	$(CC)  $(CFLAGS)   -c $< -o $@

ggml-backend.o: ggml-backend.c ggml.h ggml-backend.h
	$(CC)  $(CFLAGS)   -c $< -o $@

ggml-quants.o: ggml-quants.c ggml.h ggml-quants.h
	$(CC)  $(CFLAGS)   -c $< -o $@

WHISPER_OBJ += ggml.o ggml-alloc.o ggml-backend.o ggml-quants.o

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

ifdef WHISPER_METAL
ggml-metal.o: ggml-metal.m ggml-metal.h
	$(CC) $(CFLAGS) -c $< -o $@

WHISPER_OBJ += ggml-metal.o

ifdef WHISPER_METAL_EMBED_LIBRARY
CFLAGS += -DGGML_METAL_EMBED_LIBRARY

ggml-metal-embed.o: ggml-metal.metal
	@echo "Embedding Metal library"
	$(eval TEMP_ASSEMBLY=$(shell mktemp))
	@echo ".section __DATA, __ggml_metallib" > $(TEMP_ASSEMBLY)
	@echo ".globl _ggml_metallib_start" >> $(TEMP_ASSEMBLY)
	@echo "_ggml_metallib_start:" >> $(TEMP_ASSEMBLY)
	@echo ".incbin \"$<\"" >> $(TEMP_ASSEMBLY)
	@echo ".globl _ggml_metallib_end" >> $(TEMP_ASSEMBLY)
	@echo "_ggml_metallib_end:" >> $(TEMP_ASSEMBLY)
	@$(AS) $(TEMP_ASSEMBLY) -o $@
	@rm -f ${TEMP_ASSEMBLY}

WHISPER_OBJ += ggml-metal-embed.o
endif
endif

libwhisper.a: $(WHISPER_OBJ)
	$(AR) rcs libwhisper.a $(WHISPER_OBJ)

libwhisper.so: $(WHISPER_OBJ)
	$(CXX) $(CXXFLAGS) -shared -o libwhisper.so $(WHISPER_OBJ) $(LDFLAGS)

clean:
	rm -f *.o main stream command talk talk-llama bench quantize server lsp libwhisper.a libwhisper.so

#
# Examples
#

CC_SDL=`sdl2-config --cflags --libs`

SRC_COMMON     = examples/common.cpp examples/common-ggml.cpp
SRC_COMMON_SDL = examples/common-sdl.cpp

main: examples/main/main.cpp $(SRC_COMMON) $(WHISPER_OBJ)
	$(CXX) $(CXXFLAGS) examples/main/main.cpp $(SRC_COMMON) $(WHISPER_OBJ) -o main $(LDFLAGS)
	./main -h

bench: examples/bench/bench.cpp $(WHISPER_OBJ)
	$(CXX) $(CXXFLAGS) examples/bench/bench.cpp $(WHISPER_OBJ) -o bench $(LDFLAGS)

quantize: examples/quantize/quantize.cpp $(WHISPER_OBJ) $(SRC_COMMON)
	$(CXX) $(CXXFLAGS) examples/quantize/quantize.cpp $(SRC_COMMON) $(WHISPER_OBJ) -o quantize $(LDFLAGS)

server: examples/server/server.cpp $(SRC_COMMON) $(WHISPER_OBJ)
	$(CXX) $(CXXFLAGS) examples/server/server.cpp $(SRC_COMMON) $(WHISPER_OBJ) -o server $(LDFLAGS) $(LWINSOCK2)

stream: examples/stream/stream.cpp $(SRC_COMMON) $(SRC_COMMON_SDL) $(WHISPER_OBJ)
	$(CXX) $(CXXFLAGS) examples/stream/stream.cpp $(SRC_COMMON) $(SRC_COMMON_SDL) $(WHISPER_OBJ) -o stream $(CC_SDL) $(LDFLAGS)

command: examples/command/command.cpp examples/grammar-parser.cpp $(SRC_COMMON) $(SRC_COMMON_SDL) $(WHISPER_OBJ)
	$(CXX) $(CXXFLAGS) examples/command/command.cpp examples/grammar-parser.cpp $(SRC_COMMON) $(SRC_COMMON_SDL) $(WHISPER_OBJ) -o command $(CC_SDL) $(LDFLAGS)

lsp: examples/lsp/lsp.cpp $(SRC_COMMON) $(SRC_COMMON_SDL) $(WHISPER_OBJ)
	$(CXX) $(CXXFLAGS) examples/lsp/lsp.cpp $(SRC_COMMON) $(SRC_COMMON_SDL) $(WHISPER_OBJ) -o lsp $(CC_SDL) $(LDFLAGS)

talk: examples/talk/talk.cpp examples/talk/gpt-2.cpp $(SRC_COMMON) $(SRC_COMMON_SDL) $(WHISPER_OBJ)
	$(CXX) $(CXXFLAGS) examples/talk/talk.cpp examples/talk/gpt-2.cpp $(SRC_COMMON) $(SRC_COMMON_SDL) $(WHISPER_OBJ) -o talk $(CC_SDL) $(LDFLAGS)

talk-llama: examples/talk-llama/talk-llama.cpp examples/talk-llama/llama.cpp examples/talk-llama/unicode.cpp $(SRC_COMMON) $(SRC_COMMON_SDL) $(WHISPER_OBJ)
	$(CXX) $(CXXFLAGS) examples/talk-llama/talk-llama.cpp examples/talk-llama/llama.cpp examples/talk-llama/unicode.cpp $(SRC_COMMON) $(SRC_COMMON_SDL) $(WHISPER_OBJ) -o talk-llama $(CC_SDL) $(LDFLAGS)

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
.PHONY: large-v2
.PHONY: large-v3

tiny.en tiny base.en base small.en small medium.en medium large-v1 large-v2 large-v3: main
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
