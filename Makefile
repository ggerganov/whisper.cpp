ifndef UNAME_S
UNAME_S := $(shell uname -s)
endif

ifndef UNAME_P
UNAME_P := $(shell uname -p)
endif

ifndef UNAME_M
UNAME_M := $(shell uname -m)
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

CFLAGS   = -I.              -O3 -std=c11   -fPIC
CXXFLAGS = -I. -I./examples -O3 -std=c++11 -fPIC
LDFLAGS  =

# OS specific
# TODO: support Windows
ifeq ($(UNAME_S),Linux)
	CFLAGS   += -pthread
	CXXFLAGS += -pthread
endif
ifeq ($(UNAME_S),Darwin)
	CFLAGS   += -pthread
	CXXFLAGS += -pthread
endif
ifeq ($(UNAME_S),FreeBSD)
	CFLAGS   += -pthread
	CXXFLAGS += -pthread
endif
ifeq ($(UNAME_S),Haiku)
	CFLAGS   += -pthread
	CXXFLAGS += -pthread
endif

# Architecture specific
# TODO: probably these flags need to be tweaked on some architectures
#       feel free to update the Makefile for your architecture and send a pull request or issue
ifeq ($(UNAME_M),$(filter $(UNAME_M),x86_64 i686))
	ifeq ($(UNAME_S),Darwin)
		CFLAGS += -mf16c
		AVX1_M := $(shell sysctl machdep.cpu.features)
		ifneq (,$(findstring FMA,$(AVX1_M)))
			CFLAGS += -mfma
		endif
		ifneq (,$(findstring AVX1.0,$(AVX1_M)))
			CFLAGS += -mavx
		endif
		AVX2_M := $(shell sysctl machdep.cpu.leaf7_features)
		ifneq (,$(findstring AVX2,$(AVX2_M)))
			CFLAGS += -mavx2
		endif
	else ifeq ($(UNAME_S),Linux)
		AVX1_M := $(shell grep "avx " /proc/cpuinfo)
		ifneq (,$(findstring avx,$(AVX1_M)))
			CFLAGS += -mavx
		endif
		AVX2_M := $(shell grep "avx2 " /proc/cpuinfo)
		ifneq (,$(findstring avx2,$(AVX2_M)))
			CFLAGS += -mavx2
		endif
		FMA_M := $(shell grep "fma " /proc/cpuinfo)
		ifneq (,$(findstring fma,$(FMA_M)))
			CFLAGS += -mfma
		endif
		F16C_M := $(shell grep "f16c " /proc/cpuinfo)
		ifneq (,$(findstring f16c,$(F16C_M)))
			CFLAGS += -mf16c
		endif
		SSE3_M := $(shell grep "sse3 " /proc/cpuinfo)
		ifneq (,$(findstring sse3,$(SSE3_M)))
			CFLAGS += -msse3
		endif
	else ifeq ($(UNAME_S),Haiku)
		AVX1_M := $(shell sysinfo -cpu | grep "AVX ")
		ifneq (,$(findstring avx,$(AVX1_M)))
			CFLAGS += -mavx
		endif
		AVX2_M := $(shell sysinfo -cpu | grep "AVX2 ")
		ifneq (,$(findstring avx2,$(AVX2_M)))
			CFLAGS += -mavx2
		endif
		FMA_M := $(shell sysinfo -cpu | grep "FMA ")
		ifneq (,$(findstring fma,$(FMA_M)))
			CFLAGS += -mfma
		endif
		F16C_M := $(shell sysinfo -cpu | grep "F16C ")
		ifneq (,$(findstring f16c,$(F16C_M)))
			CFLAGS += -mf16c
		endif
	else
		CFLAGS += -mfma -mf16c -mavx -mavx2
	endif
endif
ifeq ($(UNAME_M),amd64)
	CFLAGS += -mavx -mavx2 -mfma -mf16c
endif
ifeq ($(UNAME_M),ppc64le)
	POWER9_M := $(shell grep "POWER9" /proc/cpuinfo)
	ifneq (,$(findstring POWER9,$(POWER9_M)))
		CFLAGS += -mpower9-vector
	endif
endif
ifndef WHISPER_NO_ACCELERATE
	# Mac M1 - include Accelerate framework
	ifeq ($(UNAME_S),Darwin)
		CFLAGS  += -DGGML_USE_ACCELERATE
		LDFLAGS += -framework Accelerate
	endif
endif
ifdef WHISPER_OPENBLAS
	CFLAGS  += -DGGML_USE_OPENBLAS -I/usr/local/include/openblas
	LDFLAGS += -lopenblas
endif
ifdef WHISPER_GPROF
	CFLAGS  += -pg
	CXXFLAGS  += -pg
endif
ifneq ($(filter aarch64%,$(UNAME_M)),)
endif
ifneq ($(filter armv6%,$(UNAME_M)),)
	# Raspberry Pi 1, 2, 3
	CFLAGS += -mfpu=neon-fp-armv8 -mfp16-format=ieee -mno-unaligned-access
endif
ifneq ($(filter armv7%,$(UNAME_M)),)
	# Raspberry Pi 4
	CFLAGS += -mfpu=neon-fp-armv8 -mfp16-format=ieee -mno-unaligned-access -funsafe-math-optimizations
endif
ifneq ($(filter armv8%,$(UNAME_M)),)
	# Raspberry Pi 4
	CFLAGS += -mfp16-format=ieee -mno-unaligned-access
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

default: main

#
# Build library
#

ggml.o: ggml.c ggml.h
	$(CC)  $(CFLAGS)   -c ggml.c -o ggml.o

whisper.o: whisper.cpp whisper.h
	$(CXX) $(CXXFLAGS) -c whisper.cpp -o whisper.o

libwhisper.a: ggml.o whisper.o
	$(AR) rcs libwhisper.a ggml.o whisper.o

libwhisper.so: ggml.o whisper.o
	$(CXX) $(CXXFLAGS) -shared -o libwhisper.so ggml.o whisper.o $(LDFLAGS)

clean:
	rm -f *.o main stream command talk bench libwhisper.a libwhisper.so

#
# Examples
#

CC_SDL=`sdl2-config --cflags --libs`

main: examples/main/main.cpp ggml.o whisper.o
	$(CXX) $(CXXFLAGS) examples/main/main.cpp ggml.o whisper.o -o main $(LDFLAGS)
	./main -h

stream: examples/stream/stream.cpp ggml.o whisper.o
	$(CXX) $(CXXFLAGS) examples/stream/stream.cpp ggml.o whisper.o -o stream $(CC_SDL) $(LDFLAGS)

command: examples/command/command.cpp ggml.o whisper.o
	$(CXX) $(CXXFLAGS) examples/command/command.cpp ggml.o whisper.o -o command $(CC_SDL) $(LDFLAGS)

talk: examples/talk/talk.cpp  examples/talk/gpt-2.cpp ggml.o whisper.o
	$(CXX) $(CXXFLAGS) examples/talk/talk.cpp examples/talk/gpt-2.cpp ggml.o whisper.o -o talk $(CC_SDL) $(LDFLAGS)

bench: examples/bench/bench.cpp ggml.o whisper.o
	$(CXX) $(CXXFLAGS) examples/bench/bench.cpp ggml.o whisper.o -o bench $(LDFLAGS)

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
	@echo "Converting to 16-bit WAV ..."
	@ffmpeg -loglevel -0 -y -i samples/gb0.ogg -ar 16000 -ac 1 -c:a pcm_s16le samples/gb0.wav
	@ffmpeg -loglevel -0 -y -i samples/gb1.ogg -ar 16000 -ac 1 -c:a pcm_s16le samples/gb1.wav
	@ffmpeg -loglevel -0 -y -i samples/hp0.ogg -ar 16000 -ac 1 -c:a pcm_s16le samples/hp0.wav
	@ffmpeg -loglevel -0 -y -i samples/mm1.wav -ar 16000 -ac 1 -c:a pcm_s16le samples/mm0.wav
	@rm samples/mm1.wav

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
	bash ./tests/run-tests.sh
