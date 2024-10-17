# Define the default target now so that it is always the first target
BUILD_TARGETS = \
	main \
	bench \
	quantize \
	server

# Binaries only useful for tests
TEST_TARGETS = \
	tests/test-c.o

# Deprecation aliases
ifdef WHISPER_CUBLAS
$(error WHISPER_CUBLAS is removed. Use GGML_CUDA instead.)
endif

ifdef WHISPER_CUDA
GGML_CUDA := 1
DEPRECATE_WARNING := 1
endif

ifdef WHISPER_KOMPUTE
GGML_KOMPUTE := 1
DEPRECATE_WARNING := 1
endif

ifdef WHISPER_METAL
GGML_METAL := 1
DEPRECATE_WARNING := 1
endif

ifdef WHISPER_OPENMP
GGML_OPENMP := 1
DEPRECATE_WARNING := 1
endif

ifdef WHISPER_RPC
GGML_RPC := 1
DEPRECATE_WARNING := 1
endif

ifdef WHISPER_SYCL
GGML_SYCL := 1
DEPRECATE_WARNING := 1
endif

ifdef WHISPER_SYCL_F16
GGML_SYCL_F16 := 1
DEPRECATE_WARNING := 1
endif

ifdef WHISPER_OPENBLAS
GGML_OPENBLAS := 1
DEPRECATE_WARNING := 1
endif

ifdef WHISPER_OPENBLAS64
GGML_OPENBLAS64 := 1
DEPRECATE_WARNING := 1
endif

ifdef WHISPER_BLIS
GGML_BLIS := 1
DEPRECATE_WARNING := 1
endif

ifdef WHISPER_NO_WHISPERFILE
GGML_NO_WHISPERFILE := 1
DEPRECATE_WARNING := 1
endif

ifdef WHISPER_NO_ACCELERATE
GGML_NO_ACCELERATE := 1
DEPRECATE_WARNING := 1
endif

ifdef WHISPER_NO_OPENMP
GGML_NO_OPENMP := 1
DEPRECATE_WARNING := 1
endif

ifdef WHISPER_NO_METAL
GGML_NO_METAL := 1
DEPRECATE_WARNING := 1
endif

ifndef UNAME_S
UNAME_S := $(shell uname -s)
endif

ifndef UNAME_P
UNAME_P := $(shell uname -p)
endif

ifndef UNAME_M
UNAME_M := $(shell uname -m)
endif

# In GNU make default CXX is g++ instead of c++.  Let's fix that so that users
# of non-gcc compilers don't have to provide g++ alias or wrapper.
DEFCC  := cc
DEFCXX := c++
ifeq ($(origin CC),default)
CC  := $(DEFCC)
endif
ifeq ($(origin CXX),default)
CXX := $(DEFCXX)
endif

# Mac OS + Arm can report x86_64
# ref: https://github.com/ggerganov/whisper.cpp/issues/66#issuecomment-1282546789
ifeq ($(UNAME_S),Darwin)
	ifndef GGML_NO_METAL
		GGML_METAL := 1
	endif

	GGML_NO_OPENMP := 1

	ifneq ($(UNAME_P),arm)
		SYSCTL_M := $(shell sysctl -n hw.optional.arm64 2>/dev/null)
		ifeq ($(SYSCTL_M),1)
			# UNAME_P := arm
			# UNAME_M := arm64
			warn := $(warning Your arch is announced as x86_64, but it seems to actually be ARM64. Not fixing that can lead to bad performance. For more info see: https://github.com/ggerganov/whisper.cpp/issues/66\#issuecomment-1282546789)
		endif
	endif
endif

ifdef GGML_METAL
	GGML_METAL_EMBED_LIBRARY := 1
endif

ifdef GGML_RPC
	BUILD_TARGETS += rpc-server
endif

ifdef GGML_VULKAN
	BUILD_TARGETS += vulkan-shaders-gen
endif

ifeq ($(shell sdl2-config --cflags --libs 2>/dev/null),)
else
	BUILD_TARGETS += \
		command \
		stream \
		lsp \
		talk-llama
	# talk (TODO: disalbed)
endif

default: $(BUILD_TARGETS)

test: $(TEST_TARGETS)
	@failures=0; \
	for test_target in $(TEST_TARGETS); do \
		echo "Running test $$test_target..."; \
		./$$test_target; \
		if [ $$? -ne 0 ]; then \
			printf 'Test %s FAILED!\n\n' $$test_target; \
			failures=$$(( failures + 1 )); \
		else \
			printf 'Test %s passed.\n\n' $$test_target; \
		fi; \
	done; \
	failures=$$(( failures + $$? )); \
	if [ $$failures -gt 0 ]; then \
		printf '\n%s tests failed.\n' $$failures; \
		exit 1; \
	fi
	@echo 'All tests passed.'

all: $(BUILD_TARGETS) $(TEST_TARGETS)

ifdef RISCV_CROSS_COMPILE
	CC	:= riscv64-unknown-linux-gnu-gcc
	CXX	:= riscv64-unknown-linux-gnu-g++
endif

#
# Compile flags
#

# keep standard at C11 and C++11
MK_CPPFLAGS  = -Iggml/include -Iggml/src -Iinclude -Isrc -Iexamples
MK_CFLAGS    = -std=c11   -fPIC
MK_CXXFLAGS  = -std=c++11 -fPIC
MK_NVCCFLAGS = -std=c++11

ifndef WHISPER_NO_CCACHE
CCACHE := $(shell which ccache)
ifdef CCACHE
export CCACHE_SLOPPINESS = time_macros
$(info I ccache found, compilation results will be cached. Disable with WHISPER_NO_CCACHE.)
CC    := $(CCACHE) $(CC)
CXX   := $(CCACHE) $(CXX)
else
$(info I ccache not found. Consider installing it for faster compilation.)
endif # CCACHE
endif # WHISPER_NO_CCACHE

# clock_gettime came in POSIX.1b (1993)
# CLOCK_MONOTONIC came in POSIX.1-2001 / SUSv3 as optional
# posix_memalign came in POSIX.1-2001 / SUSv3
# M_PI is an XSI extension since POSIX.1-2001 / SUSv3, came in XPG1 (1985)
MK_CPPFLAGS += -D_XOPEN_SOURCE=600

# Somehow in OpenBSD whenever POSIX conformance is specified
# some string functions rely on locale_t availability,
# which was introduced in POSIX.1-2008, forcing us to go higher
ifeq ($(UNAME_S),OpenBSD)
	MK_CPPFLAGS += -U_XOPEN_SOURCE -D_XOPEN_SOURCE=700
endif

# Data types, macros and functions related to controlling CPU affinity and
# some memory allocation are available on Linux through GNU extensions in libc
ifeq ($(UNAME_S),Linux)
	MK_CPPFLAGS += -D_GNU_SOURCE
endif

# RLIMIT_MEMLOCK came in BSD, is not specified in POSIX.1,
# and on macOS its availability depends on enabling Darwin extensions
# similarly on DragonFly, enabling BSD extensions is necessary
ifeq ($(UNAME_S),Darwin)
	MK_CPPFLAGS += -D_DARWIN_C_SOURCE
endif
ifeq ($(UNAME_S),DragonFly)
	MK_CPPFLAGS += -D__BSD_VISIBLE
endif

# alloca is a non-standard interface that is not visible on BSDs when
# POSIX conformance is specified, but not all of them provide a clean way
# to enable it in such cases
ifeq ($(UNAME_S),FreeBSD)
	MK_CPPFLAGS += -D__BSD_VISIBLE
endif
ifeq ($(UNAME_S),NetBSD)
	MK_CPPFLAGS += -D_NETBSD_SOURCE
endif
ifeq ($(UNAME_S),OpenBSD)
	MK_CPPFLAGS += -D_BSD_SOURCE
endif

ifdef GGML_SCHED_MAX_COPIES
	MK_CPPFLAGS += -DGGML_SCHED_MAX_COPIES=$(GGML_SCHED_MAX_COPIES)
endif

ifdef WHISPER_DEBUG
	MK_CFLAGS    += -O0 -g
	MK_CXXFLAGS  += -O0 -g
	MK_LDFLAGS   += -g
	MK_NVCCFLAGS += -O0 -g

	ifeq ($(UNAME_S),Linux)
		MK_CPPFLAGS += -D_GLIBCXX_ASSERTIONS
	endif
else
	MK_CPPFLAGS   += -DNDEBUG
	MK_CFLAGS     += -O3
	MK_CXXFLAGS   += -O3
	MK_NVCCFLAGS  += -O3
endif

ifdef WHISPER_SANITIZE_THREAD
	MK_CFLAGS   += -fsanitize=thread -g
	MK_CXXFLAGS += -fsanitize=thread -g
	MK_LDFLAGS  += -fsanitize=thread -g
endif

ifdef WHISPER_SANITIZE_ADDRESS
	MK_CFLAGS   += -fsanitize=address -fno-omit-frame-pointer -g
	MK_CXXFLAGS += -fsanitize=address -fno-omit-frame-pointer -g
	MK_LDFLAGS  += -fsanitize=address -fno-omit-frame-pointer -g
endif

ifdef WHISPER_SANITIZE_UNDEFINED
	MK_CFLAGS   += -fsanitize=undefined -g
	MK_CXXFLAGS += -fsanitize=undefined -g
	MK_LDFLAGS  += -fsanitize=undefined -g
endif

ifdef WHISPER_SERVER_VERBOSE
	MK_CPPFLAGS += -DSERVER_VERBOSE=$(WHISPER_SERVER_VERBOSE)
endif

ifdef WHISPER_SERVER_SSL
	MK_CPPFLAGS += -DCPPHTTPLIB_OPENSSL_SUPPORT
	MK_LDFLAGS  += -lssl -lcrypto
endif

ifdef WHISPER_DISABLE_LOGS
	MK_CPPFLAGS += -DLOG_DISABLE_LOGS
endif # WHISPER_DISABLE_LOGS

# warnings
WARN_FLAGS = \
	-Wall \
	-Wextra \
	-Wpedantic \
	-Wcast-qual \
	-Wno-unused-function

MK_CFLAGS += \
	$(WARN_FLAGS) \
	-Wshadow \
	-Wstrict-prototypes \
	-Wpointer-arith \
	-Wmissing-prototypes \
	-Werror=implicit-int \
	-Werror=implicit-function-declaration

MK_CXXFLAGS += \
	$(WARN_FLAGS) \
	-Wmissing-declarations \
	-Wmissing-noreturn

ifeq ($(WHISPER_FATAL_WARNINGS),1)
	MK_CFLAGS   += -Werror
	MK_CXXFLAGS += -Werror
endif

# this version of Apple ld64 is buggy
ifneq '' '$(findstring dyld-1015.7,$(shell $(CC) $(LDFLAGS) -Wl,-v 2>&1))'
	MK_CPPFLAGS += -DHAVE_BUGGY_APPLE_LINKER
endif

# OS specific
# TODO: support Windows
ifneq '' '$(filter $(UNAME_S),Linux Darwin FreeBSD NetBSD OpenBSD Haiku)'
	MK_CFLAGS   += -pthread
	MK_CXXFLAGS += -pthread
endif

# detect Windows
ifneq ($(findstring _NT,$(UNAME_S)),)
	_WIN32 := 1
endif

# library name prefix
ifneq ($(_WIN32),1)
	LIB_PRE := lib
endif

# Dynamic Shared Object extension
ifneq ($(_WIN32),1)
	DSO_EXT := .so
else
	DSO_EXT := .dll
endif

# Windows Sockets 2 (Winsock) for network-capable apps
ifeq ($(_WIN32),1)
	LWINSOCK2 := -lws2_32
endif

ifdef WHISPER_GPROF
	MK_CFLAGS   += -pg
	MK_CXXFLAGS += -pg
endif

# Architecture specific
# TODO: probably these flags need to be tweaked on some architectures
#       feel free to update the Makefile for your architecture and send a pull request or issue

ifndef RISCV

ifeq ($(UNAME_M),$(filter $(UNAME_M),x86_64 i686 amd64))
	# Use all CPU extensions that are available:
	MK_CFLAGS     += -march=native -mtune=native
	HOST_CXXFLAGS += -march=native -mtune=native

	# Usage AVX-only
	#MK_CFLAGS   += -mfma -mf16c -mavx
	#MK_CXXFLAGS += -mfma -mf16c -mavx

	# Usage SSSE3-only (Not is SSE3!)
	#MK_CFLAGS   += -mssse3
	#MK_CXXFLAGS += -mssse3
endif

ifneq '' '$(findstring mingw,$(shell $(CC) -dumpmachine))'
	# The stack is only 16-byte aligned on Windows, so don't let gcc emit aligned moves.
	# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=54412
	# https://github.com/ggerganov/llama.cpp/issues/2922
	MK_CFLAGS   += -Xassembler -muse-unaligned-vector-move
	MK_CXXFLAGS += -Xassembler -muse-unaligned-vector-move

	# Target Windows 8 for PrefetchVirtualMemory
	MK_CPPFLAGS += -D_WIN32_WINNT=0x602
endif

ifneq ($(filter aarch64%,$(UNAME_M)),)
	# Apple M1, M2, etc.
	# Raspberry Pi 3, 4, Zero 2 (64-bit)
	# Nvidia Jetson
	MK_CFLAGS   += -mcpu=native
	MK_CXXFLAGS += -mcpu=native
	JETSON_RELEASE_INFO = $(shell jetson_release)
	ifdef JETSON_RELEASE_INFO
		ifneq ($(filter TX2%,$(JETSON_RELEASE_INFO)),)
			JETSON_EOL_MODULE_DETECT = 1
			CC = aarch64-unknown-linux-gnu-gcc
			cxx = aarch64-unknown-linux-gnu-g++
		endif
	endif
endif

ifneq ($(filter armv6%,$(UNAME_M)),)
	# Raspberry Pi 1, Zero
	MK_CFLAGS   += -mfpu=neon-fp-armv8 -mfp16-format=ieee -mno-unaligned-access
	MK_CXXFLAGS += -mfpu=neon-fp-armv8 -mfp16-format=ieee -mno-unaligned-access
endif

ifneq ($(filter armv7%,$(UNAME_M)),)
	# Raspberry Pi 2
	MK_CFLAGS   += -mfpu=neon-fp-armv8 -mfp16-format=ieee -mno-unaligned-access -funsafe-math-optimizations
	MK_CXXFLAGS += -mfpu=neon-fp-armv8 -mfp16-format=ieee -mno-unaligned-access -funsafe-math-optimizations
endif

ifneq ($(filter armv8%,$(UNAME_M)),)
	# Raspberry Pi 3, 4, Zero 2 (32-bit)
	MK_CFLAGS   += -mfp16-format=ieee -mno-unaligned-access
	MK_CXXFLAGS += -mfp16-format=ieee -mno-unaligned-access
endif

ifneq ($(filter ppc64%,$(UNAME_M)),)
	POWER9_M := $(shell grep "POWER9" /proc/cpuinfo)
	ifneq (,$(findstring POWER9,$(POWER9_M)))
		MK_CFLAGS   += -mcpu=power9
		MK_CXXFLAGS += -mcpu=power9
	endif
endif

ifneq ($(filter ppc64le%,$(UNAME_M)),)
	MK_CFLAGS   += -mcpu=powerpc64le
	MK_CXXFLAGS += -mcpu=powerpc64le
	CUDA_POWER_ARCH = 1
endif

ifneq ($(filter loongarch64%,$(UNAME_M)),)
	MK_CFLAGS   += -mlasx
	MK_CXXFLAGS += -mlasx
endif

else
	MK_CFLAGS   += -march=rv64gcv -mabi=lp64d
	MK_CXXFLAGS += -march=rv64gcv -mabi=lp64d
endif

ifndef GGML_NO_ACCELERATE
	# Mac OS - include Accelerate framework.
	# `-framework Accelerate` works both with Apple Silicon and Mac Intel
	ifeq ($(UNAME_S),Darwin)
		MK_CPPFLAGS += -DGGML_USE_ACCELERATE -DGGML_USE_BLAS
		MK_CPPFLAGS += -DACCELERATE_NEW_LAPACK
		MK_CPPFLAGS += -DACCELERATE_LAPACK_ILP64
		MK_LDFLAGS  += -framework Accelerate
		OBJ_GGML    += ggml/src/ggml-blas.o
	endif
endif # GGML_NO_ACCELERATE

ifndef GGML_NO_OPENMP
	MK_CPPFLAGS += -DGGML_USE_OPENMP
	MK_CFLAGS   += -fopenmp
	MK_CXXFLAGS += -fopenmp
endif # GGML_NO_OPENMP

ifdef GGML_OPENBLAS
	MK_CPPFLAGS += -DGGML_USE_BLAS $(shell pkg-config --cflags-only-I openblas)
	MK_CFLAGS   += $(shell pkg-config --cflags-only-other openblas)
	MK_LDFLAGS  += $(shell pkg-config --libs openblas)
	OBJ_GGML    += ggml/src/ggml-blas.o
endif # GGML_OPENBLAS

ifdef GGML_OPENBLAS64
	MK_CPPFLAGS += -DGGML_USE_BLAS $(shell pkg-config --cflags-only-I openblas64)
	MK_CFLAGS   += $(shell pkg-config --cflags-only-other openblas64)
	MK_LDFLAGS  += $(shell pkg-config --libs openblas64)
	OBJ_GGML    += ggml/src/ggml-blas.o
endif # GGML_OPENBLAS64

ifdef GGML_BLIS
	MK_CPPFLAGS += -DGGML_USE_BLAS -I/usr/local/include/blis -I/usr/include/blis
	MK_LDFLAGS  += -lblis -L/usr/local/lib
	OBJ_GGML    += ggml/src/ggml-blas.o
endif # GGML_BLIS

ifdef GGML_RPC
	MK_CPPFLAGS += -DGGML_USE_RPC
	OBJ_GGML    += ggml/src/ggml-rpc.o
endif # GGML_RPC

OBJ_CUDA_TMPL      = $(patsubst %.cu,%.o,$(wildcard ggml/src/ggml-cuda/template-instances/fattn-wmma*.cu))
OBJ_CUDA_TMPL     += $(patsubst %.cu,%.o,$(wildcard ggml/src/ggml-cuda/template-instances/mmq*.cu))

ifdef GGML_CUDA_FA_ALL_QUANTS
	OBJ_CUDA_TMPL += $(patsubst %.cu,%.o,$(wildcard ggml/src/ggml-cuda/template-instances/fattn-vec*.cu))
else
	OBJ_CUDA_TMPL += $(patsubst %.cu,%.o,$(wildcard ggml/src/ggml-cuda/template-instances/fattn-vec*q4_0-q4_0.cu))
	OBJ_CUDA_TMPL += $(patsubst %.cu,%.o,$(wildcard ggml/src/ggml-cuda/template-instances/fattn-vec*q8_0-q8_0.cu))
	OBJ_CUDA_TMPL += $(patsubst %.cu,%.o,$(wildcard ggml/src/ggml-cuda/template-instances/fattn-vec*f16-f16.cu))
endif # GGML_CUDA_FA_ALL_QUANTS

ifdef GGML_CUDA
	ifneq ('', '$(wildcard /opt/cuda)')
		CUDA_PATH ?= /opt/cuda
	else
		CUDA_PATH ?= /usr/local/cuda
	endif

	#MK_CPPFLAGS  += -DGGML_USE_CUDA -I$(CUDA_PATH)/include -I$(CUDA_PATH)/targets/$(UNAME_M)-linux/include -DGGML_CUDA_USE_GRAPHS
	#MK_LDFLAGS   += -lcuda -lcublas -lculibos -lcudart -lcufft -lcublasLt -lpthread -ldl -lrt -L$(CUDA_PATH)/lib64 -L/usr/lib64 -L$(CUDA_PATH)/targets/$(UNAME_M)-linux/lib -L$(CUDA_PATH)/lib64/stubs -L/usr/lib/wsl/lib
	MK_CPPFLAGS  += -DGGML_USE_CUDA -I$(CUDA_PATH)/include -I$(CUDA_PATH)/targets/$(UNAME_M)-linux/include
	MK_LDFLAGS   += -lcuda -lcublas -lculibos -lcudart -lcublasLt -lpthread -ldl -lrt -L$(CUDA_PATH)/lib64 -L/usr/lib64 -L$(CUDA_PATH)/targets/$(UNAME_M)-linux/lib -L$(CUDA_PATH)/lib64/stubs -L/usr/lib/wsl/lib
	MK_NVCCFLAGS += -use_fast_math

	OBJ_GGML += ggml/src/ggml-cuda.o
	OBJ_GGML += $(patsubst %.cu,%.o,$(wildcard ggml/src/ggml-cuda/*.cu))
	OBJ_GGML += $(OBJ_CUDA_TMPL)
ifdef WHISPER_FATAL_WARNINGS
	MK_NVCCFLAGS += -Werror all-warnings
endif # WHISPER_FATAL_WARNINGS

ifndef JETSON_EOL_MODULE_DETECT
	MK_NVCCFLAGS += --forward-unknown-to-host-compiler
endif # JETSON_EOL_MODULE_DETECT

ifdef WHISPER_DEBUG
	MK_NVCCFLAGS += -lineinfo
endif # WHISPER_DEBUG

ifdef GGML_CUDA_DEBUG
	MK_NVCCFLAGS += --device-debug
endif # GGML_CUDA_DEBUG

ifdef GGML_CUDA_NVCC
	NVCC = $(CCACHE) $(GGML_CUDA_NVCC)
else
	NVCC = $(CCACHE) nvcc
endif #GGML_CUDA_NVCC

ifdef CUDA_DOCKER_ARCH
	MK_NVCCFLAGS += -Wno-deprecated-gpu-targets -arch=$(CUDA_DOCKER_ARCH)
else ifndef CUDA_POWER_ARCH
	MK_NVCCFLAGS += -arch=native
endif # CUDA_DOCKER_ARCH

ifdef GGML_CUDA_FORCE_DMMV
	MK_NVCCFLAGS += -DGGML_CUDA_FORCE_DMMV
endif # GGML_CUDA_FORCE_DMMV

ifdef GGML_CUDA_FORCE_MMQ
	MK_NVCCFLAGS += -DGGML_CUDA_FORCE_MMQ
endif # GGML_CUDA_FORCE_MMQ

ifdef GGML_CUDA_DMMV_X
	MK_NVCCFLAGS += -DGGML_CUDA_DMMV_X=$(GGML_CUDA_DMMV_X)
else
	MK_NVCCFLAGS += -DGGML_CUDA_DMMV_X=32
endif # GGML_CUDA_DMMV_X

ifdef GGML_CUDA_MMV_Y
	MK_NVCCFLAGS += -DGGML_CUDA_MMV_Y=$(GGML_CUDA_MMV_Y)
else ifdef GGML_CUDA_DMMV_Y
	MK_NVCCFLAGS += -DGGML_CUDA_MMV_Y=$(GGML_CUDA_DMMV_Y) # for backwards compatibility
else
	MK_NVCCFLAGS += -DGGML_CUDA_MMV_Y=1
endif # GGML_CUDA_MMV_Y

ifdef GGML_CUDA_F16
	MK_NVCCFLAGS += -DGGML_CUDA_F16
endif # GGML_CUDA_F16

ifdef GGML_CUDA_DMMV_F16
	MK_NVCCFLAGS += -DGGML_CUDA_F16
endif # GGML_CUDA_DMMV_F16

ifdef GGML_CUDA_KQUANTS_ITER
	MK_NVCCFLAGS += -DK_QUANTS_PER_ITERATION=$(GGML_CUDA_KQUANTS_ITER)
else
	MK_NVCCFLAGS += -DK_QUANTS_PER_ITERATION=2
endif

ifdef GGML_CUDA_PEER_MAX_BATCH_SIZE
	MK_NVCCFLAGS += -DGGML_CUDA_PEER_MAX_BATCH_SIZE=$(GGML_CUDA_PEER_MAX_BATCH_SIZE)
else
	MK_NVCCFLAGS += -DGGML_CUDA_PEER_MAX_BATCH_SIZE=128
endif # GGML_CUDA_PEER_MAX_BATCH_SIZE

ifdef GGML_CUDA_NO_PEER_COPY
	MK_NVCCFLAGS += -DGGML_CUDA_NO_PEER_COPY
endif # GGML_CUDA_NO_PEER_COPY

ifdef GGML_CUDA_CCBIN
	MK_NVCCFLAGS += -ccbin $(GGML_CUDA_CCBIN)
endif # GGML_CUDA_CCBIN

ifdef GGML_CUDA_FA_ALL_QUANTS
	MK_NVCCFLAGS += -DGGML_CUDA_FA_ALL_QUANTS
endif # GGML_CUDA_FA_ALL_QUANTS

ifdef JETSON_EOL_MODULE_DETECT
define NVCC_COMPILE
	$(NVCC) -I. -Icommon -D_XOPEN_SOURCE=600 -D_GNU_SOURCE -DNDEBUG -DGGML_USE_CUDA -I/usr/local/cuda/include -I/opt/cuda/include -I/usr/local/cuda/targets/aarch64-linux/include -std=c++11 -O3 $(NVCCFLAGS) $(CPPFLAGS) -Xcompiler "$(CUDA_CXXFLAGS)" -c $< -o $@
endef # NVCC_COMPILE
else
define NVCC_COMPILE
	$(NVCC) $(NVCCFLAGS) $(CPPFLAGS) -Xcompiler "$(CUDA_CXXFLAGS)" -c $< -o $@
endef # NVCC_COMPILE
endif # JETSON_EOL_MODULE_DETECT

ggml/src/ggml-cuda/%.o: \
	ggml/src/ggml-cuda/%.cu \
	ggml/include/ggml.h \
	ggml/src/ggml-common.h \
	ggml/src/ggml-cuda/common.cuh
	$(NVCC_COMPILE)

ggml/src/ggml-cuda.o: \
	ggml/src/ggml-cuda.cu \
	ggml/include/ggml.h \
	ggml/include/ggml-backend.h \
	ggml/include/ggml-cuda.h \
	ggml/src/ggml-backend-impl.h \
	ggml/src/ggml-common.h \
	$(wildcard ggml/src/ggml-cuda/*.cuh)
	$(NVCC_COMPILE)
endif # GGML_CUDA

ifdef GGML_VULKAN
	MK_CPPFLAGS += -DGGML_USE_VULKAN
	MK_LDFLAGS  += $(shell pkg-config --libs vulkan)
	OBJ_GGML    += ggml/src/ggml-vulkan.o ggml/src/ggml-vulkan-shaders.o

ifdef GGML_VULKAN_CHECK_RESULTS
	MK_CPPFLAGS  += -DGGML_VULKAN_CHECK_RESULTS
endif

ifdef GGML_VULKAN_DEBUG
	MK_CPPFLAGS  += -DGGML_VULKAN_DEBUG
endif

ifdef GGML_VULKAN_MEMORY_DEBUG
	MK_CPPFLAGS  += -DGGML_VULKAN_MEMORY_DEBUG
endif

ifdef GGML_VULKAN_PERF
	MK_CPPFLAGS  += -DGGML_VULKAN_PERF
endif

ifdef GGML_VULKAN_VALIDATE
	MK_CPPFLAGS  += -DGGML_VULKAN_VALIDATE
endif

ifdef GGML_VULKAN_RUN_TESTS
	MK_CPPFLAGS  += -DGGML_VULKAN_RUN_TESTS
endif

GLSLC_CMD  = glslc
_ggml_vk_genshaders_cmd = $(shell pwd)/vulkan-shaders-gen
_ggml_vk_header = ggml/src/ggml-vulkan-shaders.hpp
_ggml_vk_source = ggml/src/ggml-vulkan-shaders.cpp
_ggml_vk_input_dir = ggml/src/vulkan-shaders
_ggml_vk_shader_deps = $(echo $(_ggml_vk_input_dir)/*.comp)

ggml/src/ggml-vulkan.o: ggml/src/ggml-vulkan.cpp ggml/include/ggml-vulkan.h $(_ggml_vk_header) $(_ggml_vk_source)
	$(CXX) $(CXXFLAGS) $(shell pkg-config --cflags vulkan) -c $< -o $@

$(_ggml_vk_header): $(_ggml_vk_source)

$(_ggml_vk_source): $(_ggml_vk_shader_deps) vulkan-shaders-gen
	$(_ggml_vk_genshaders_cmd) \
		--glslc      $(GLSLC_CMD) \
		--input-dir  $(_ggml_vk_input_dir) \
		--target-hpp $(_ggml_vk_header) \
		--target-cpp $(_ggml_vk_source)

vulkan-shaders-gen: ggml/src/vulkan-shaders/vulkan-shaders-gen.cpp
	$(CXX) $(CXXFLAGS) -o $@ $(LDFLAGS) ggml/src/vulkan-shaders/vulkan-shaders-gen.cpp

endif # GGML_VULKAN

ifdef GGML_HIPBLAS
	ifeq ($(wildcard /opt/rocm),)
		ROCM_PATH      ?= /usr
		AMDGPU_TARGETS ?= $(shell $(shell which amdgpu-arch))
	else
		ROCM_PATH	?= /opt/rocm
		AMDGPU_TARGETS ?= $(shell $(ROCM_PATH)/llvm/bin/amdgpu-arch)
	endif

	GGML_CUDA_DMMV_X       ?= 32
	GGML_CUDA_MMV_Y        ?= 1
	GGML_CUDA_KQUANTS_ITER ?= 2

	MK_CPPFLAGS += -DGGML_USE_HIPBLAS -DGGML_USE_CUDA

ifdef GGML_HIP_UMA
	MK_CPPFLAGS += -DGGML_HIP_UMA
endif # GGML_HIP_UMA

	MK_LDFLAGS += -L$(ROCM_PATH)/lib -Wl,-rpath=$(ROCM_PATH)/lib
	MK_LDFLAGS += -L$(ROCM_PATH)/lib64 -Wl,-rpath=$(ROCM_PATH)/lib64
	MK_LDFLAGS += -lhipblas -lamdhip64 -lrocblas

	HIPCC ?= $(CCACHE) $(ROCM_PATH)/bin/hipcc

	HIPFLAGS += $(addprefix --offload-arch=,$(AMDGPU_TARGETS))
	HIPFLAGS += -DGGML_CUDA_DMMV_X=$(GGML_CUDA_DMMV_X)
	HIPFLAGS += -DGGML_CUDA_MMV_Y=$(GGML_CUDA_MMV_Y)
	HIPFLAGS += -DK_QUANTS_PER_ITERATION=$(GGML_CUDA_KQUANTS_ITER)

ifdef GGML_CUDA_FORCE_DMMV
	HIPFLAGS += -DGGML_CUDA_FORCE_DMMV
endif # GGML_CUDA_FORCE_DMMV

ifdef GGML_CUDA_NO_PEER_COPY
	HIPFLAGS += -DGGML_CUDA_NO_PEER_COPY
endif # GGML_CUDA_NO_PEER_COPY

	OBJ_GGML += ggml/src/ggml-cuda.o
	OBJ_GGML += $(patsubst %.cu,%.o,$(wildcard ggml/src/ggml-cuda/*.cu))
	OBJ_GGML += $(OBJ_CUDA_TMPL)

ggml/src/ggml-cuda.o: \
	ggml/src/ggml-cuda.cu \
	ggml/include/ggml.h \
	ggml/include/ggml-backend.h \
	ggml/include/ggml-cuda.h \
	ggml/src/ggml-backend-impl.h \
	ggml/src/ggml-common.h \
	$(wildcard ggml/src/ggml-cuda/*.cuh)
	$(HIPCC) $(CXXFLAGS) $(HIPFLAGS) -x hip -c -o $@ $<

ggml/src/ggml-cuda/%.o: \
	ggml/src/ggml-cuda/%.cu \
	ggml/include/ggml.h \
	ggml/src/ggml-common.h \
	ggml/src/ggml-cuda/common.cuh
	$(HIPCC) $(CXXFLAGS) $(HIPFLAGS) -x hip -c -o $@ $<
endif # GGML_HIPBLAS

ifdef GGML_METAL
	MK_CPPFLAGS += -DGGML_USE_METAL
	MK_LDFLAGS  += -framework Foundation -framework Metal -framework MetalKit
	OBJ_GGML	+= ggml/src/ggml-metal.o
ifdef GGML_METAL_NDEBUG
	MK_CPPFLAGS += -DGGML_METAL_NDEBUG
endif

ifdef GGML_METAL_EMBED_LIBRARY
	MK_CPPFLAGS += -DGGML_METAL_EMBED_LIBRARY
	OBJ_GGML    += ggml/src/ggml-metal-embed.o
endif
endif # GGML_METAL

ifdef WHISPER_COREML
	MK_CXXFLAGS += -DWHISPER_USE_COREML
	LDFLAGS     += -framework Foundation -framework CoreML

ifdef WHISPER_COREML_ALLOW_FALLBACK
	MK_CXXFLAGS += -DWHISPER_COREML_ALLOW_FALLBACK
endif
endif

# ===

ifdef GGML_METAL
ggml/src/ggml-metal.o: \
	ggml/src/ggml-metal.m \
	ggml/include/ggml-metal.h \
	ggml/include/ggml.h
	$(CC) $(CFLAGS) -c $< -o $@

ifdef GGML_METAL_EMBED_LIBRARY
ggml/src/ggml-metal-embed.o: \
	ggml/src/ggml-metal.metal \
	ggml/src/ggml-common.h
	@echo "Embedding Metal library"
	@sed -e '/#include "ggml-common.h"/r ggml/src/ggml-common.h' -e '/#include "ggml-common.h"/d' < ggml/src/ggml-metal.metal > ggml/src/ggml-metal-embed.metal
	$(eval TEMP_ASSEMBLY=$(shell mktemp))
	@echo ".section __DATA, __ggml_metallib"            >  $(TEMP_ASSEMBLY)
	@echo ".globl _ggml_metallib_start"                 >> $(TEMP_ASSEMBLY)
	@echo "_ggml_metallib_start:"                       >> $(TEMP_ASSEMBLY)
	@echo ".incbin \"ggml/src/ggml-metal-embed.metal\"" >> $(TEMP_ASSEMBLY)
	@echo ".globl _ggml_metallib_end"                   >> $(TEMP_ASSEMBLY)
	@echo "_ggml_metallib_end:"                         >> $(TEMP_ASSEMBLY)
	@$(AS) $(TEMP_ASSEMBLY) -o $@
	@rm -f ${TEMP_ASSEMBLY}
endif
endif # GGML_METAL

ifdef WHISPER_COREML
src/coreml/whisper-encoder.o: src/coreml/whisper-encoder.mm src/coreml/whisper-encoder.h
	$(CXX) -O3 -I . -fobjc-arc -c src/coreml/whisper-encoder.mm -o src/coreml/whisper-encoder.o

src/coreml/whisper-encoder-impl.o: src/coreml/whisper-encoder-impl.m src/coreml/whisper-encoder-impl.h
	$(CXX) -O3 -I . -fobjc-arc -c src/coreml/whisper-encoder-impl.m -o src/coreml/whisper-encoder-impl.o

OBJ_WHISPER += src/coreml/whisper-encoder.o src/coreml/whisper-encoder-impl.o
endif

OBJ_GGML += \
	ggml/src/ggml.o \
	ggml/src/ggml-alloc.o \
	ggml/src/ggml-backend.o \
	ggml/src/ggml-quants.o \
	ggml/src/ggml-aarch64.o

OBJ_WHISPER += \
	src/whisper.o

OBJ_COMMON += \
	examples/common.o \
	examples/common-ggml.o \
	examples/grammar-parser.o

OBJ_SDL += \
	examples/common-sdl.o

OBJ_ALL = $(OBJ_GGML) $(OBJ_WHISPER) $(OBJ_COMMON) $(OBJ_SDL)

LIB_GGML   = $(LIB_PRE)ggml$(DSO_EXT)
LIB_GGML_S = $(LIB_PRE)ggml.a

LIB_WHISPER   = $(LIB_PRE)whisper$(DSO_EXT)
LIB_WHISPER_S = $(LIB_PRE)whisper.a

LIB_COMMON   = $(LIB_PRE)common$(DSO_EXT)
LIB_COMMON_S = $(LIB_PRE)common.a

LIB_COMMON_SDL   = $(LIB_PRE)common-sdl$(DSO_EXT)
LIB_COMMON_SDL_S = $(LIB_PRE)common-sdl.a

LIB_ALL   = $(LIB_GGML)   $(LIB_WHISPER)   $(LIB_COMMON)   $(LIB_COMMON_SDL)
LIB_ALL_S = $(LIB_GGML_S) $(LIB_WHISPER_S) $(LIB_COMMON_S) $(LIB_COMMON_SDL_S)

GF_CC := $(CC)
include scripts/get-flags.mk

# combine build flags with cmdline overrides
override CPPFLAGS  := $(MK_CPPFLAGS) $(CPPFLAGS)
override CFLAGS    := $(CPPFLAGS) $(MK_CFLAGS) $(GF_CFLAGS) $(CFLAGS)
BASE_CXXFLAGS      := $(MK_CXXFLAGS) $(CXXFLAGS)
override CXXFLAGS  := $(BASE_CXXFLAGS) $(HOST_CXXFLAGS) $(GF_CXXFLAGS) $(CPPFLAGS)
override NVCCFLAGS := $(MK_NVCCFLAGS) $(NVCCFLAGS)
override LDFLAGS   := $(MK_LDFLAGS) $(LDFLAGS)

# identify CUDA host compiler
ifdef GGML_CUDA
GF_CC := $(NVCC) $(NVCCFLAGS) 2>/dev/null .c -Xcompiler
include scripts/get-flags.mk
CUDA_CXXFLAGS := $(BASE_CXXFLAGS) $(GF_CXXFLAGS) -Wno-pedantic
endif

ifdef WHISPER_CURL
override CXXFLAGS := $(CXXFLAGS) -DWHISPER_USE_CURL
override LDFLAGS  := $(LDFLAGS) -lcurl
endif

ifdef WHISPER_FFMPEG
OBJ_COMMON += examples/ffmpeg-transcode.o
override CXXFLAGS := $(CXXFLAGS) -DWHISPER_FFMPEG
override LDFLAGS  := $(LDFLAGS) -lavutil -lavformat -lavcodec -lswscale -lswresample
endif

#
# Print build information
#

$(info I whisper.cpp build info: )
$(info I UNAME_S:   $(UNAME_S))
$(info I UNAME_P:   $(UNAME_P))
$(info I UNAME_M:   $(UNAME_M))
$(info I CFLAGS:    $(CFLAGS))
$(info I CXXFLAGS:  $(CXXFLAGS))
$(info I NVCCFLAGS: $(NVCCFLAGS))
$(info I LDFLAGS:   $(LDFLAGS))
$(info I CC:        $(shell $(CC)   --version | head -n 1))
$(info I CXX:       $(shell $(CXX)  --version | head -n 1))
ifdef GGML_CUDA
$(info I NVCC:      $(shell $(NVCC) --version | tail -n 1))
CUDA_VERSION := $(shell $(NVCC) --version | grep -oP 'release (\K[0-9]+\.[0-9])')
ifeq ($(shell awk -v "v=$(CUDA_VERSION)" 'BEGIN { print (v < 11.7) }'),1)

ifndef CUDA_DOCKER_ARCH
ifndef CUDA_POWER_ARCH
$(error I ERROR: For CUDA versions < 11.7 a target CUDA architecture must be explicitly provided via environment variable CUDA_DOCKER_ARCH, e.g. by running "export CUDA_DOCKER_ARCH=compute_XX" on Unix-like systems, where XX is the minimum compute capability that the code needs to run on. A list with compute capabilities can be found here: https://developer.nvidia.com/cuda-gpus )
endif # CUDA_POWER_ARCH
endif # CUDA_DOCKER_ARCH

endif # eq ($(shell echo "$(CUDA_VERSION) < 11.7" | bc),1)
endif # GGML_CUDA
$(info )

ifdef DEPRECATE_WARNING
$(info !!! DEPRECATION WARNING !!!)
$(info The following WHISPER_ options are deprecated and will be removed in the future. Use the GGML_ prefix instead)
$(info   - WHISPER_CUDA)
$(info   - WHISPER_METAL)
$(info   - WHISPER_OPENMP)
$(info   - WHISPER_RPC)
$(info   - WHISPER_SYCL)
$(info   - WHISPER_SYCL_F16)
$(info   - WHISPER_OPENBLAS)
$(info   - WHISPER_OPENBLAS64)
$(info   - WHISPER_BLIS)
$(info   - WHISPER_NO_LLAMAFILE)
$(info   - WHISPER_NO_ACCELERATE)
$(info   - WHISPER_NO_OPENMP)
$(info   - WHISPER_NO_METAL)
$(info )
endif

#
# Build libraries
#

# ggml

ggml/src/ggml.o: \
	ggml/src/ggml.c \
	ggml/include/ggml.h
	$(CC)  $(CFLAGS)   -c $< -o $@

ggml/src/ggml-alloc.o: \
	ggml/src/ggml-alloc.c \
	ggml/include/ggml.h \
	ggml/include/ggml-alloc.h
	$(CC)  $(CFLAGS)   -c $< -o $@

ggml/src/ggml-backend.o: \
	ggml/src/ggml-backend.cpp \
	ggml/include/ggml.h \
	ggml/include/ggml-backend.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

ggml/src/ggml-quants.o: \
	ggml/src/ggml-quants.c \
	ggml/include/ggml.h \
	ggml/src/ggml-quants.h \
	ggml/src/ggml-common.h
	$(CC) $(CFLAGS)    -c $< -o $@

ggml/src/ggml-aarch64.o: \
	ggml/src/ggml-aarch64.c \
	ggml/include/ggml.h \
	ggml/src/ggml-aarch64.h \
	ggml/src/ggml-common.h
	$(CC) $(CFLAGS)    -c $< -o $@

ggml/src/ggml-blas.o: \
	ggml/src/ggml-blas.cpp \
	ggml/include/ggml-blas.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

ifdef GGML_LLAMAFILE
ggml/src/sgemm.o: \
	ggml/src/sgemm.cpp \
	ggml/src/sgemm.h \
	ggml/include/ggml.h
	$(CXX) $(CXXFLAGS) -c $< -o $@
endif # GGML_LLAMAFILE

ifdef GGML_RPC
ggml/src/ggml-rpc.o: \
	ggml/src/ggml-rpc.cpp \
	ggml/include/ggml-rpc.h
	$(CXX) $(CXXFLAGS) -c $< -o $@
endif # GGML_RPC

$(LIB_GGML): \
	$(OBJ_GGML)
	$(CXX) $(CXXFLAGS) -shared -fPIC -o $@ $^ $(LDFLAGS)

$(LIB_GGML_S): \
	$(OBJ_GGML)
	ar rcs $(LIB_GGML_S) $^

# whisper

src/whisper.o: \
	src/whisper.cpp \
	include/whisper.h \
	ggml/include/ggml.h \
	ggml/include/ggml-alloc.h \
	ggml/include/ggml-backend.h \
	ggml/include/ggml-cuda.h \
	ggml/include/ggml-metal.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(LIB_WHISPER): \
	$(OBJ_WHISPER) \
	$(LIB_GGML)
	$(CXX) $(CXXFLAGS) -shared -fPIC -o $@ $^ $(LDFLAGS)

$(LIB_WHISPER_S): \
	$(OBJ_WHISPER) \
	$(OBJ_GGML)
	ar rcs $(LIB_WHISPER_S) $^

# common

examples/common.o: \
	examples/common.cpp \
	examples/common.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

examples/common-ggml.o: \
	examples/common-ggml.cpp \
	examples/common-ggml.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(LIB_COMMON): \
	$(OBJ_COMMON)
	$(CXX) $(CXXFLAGS) -shared -fPIC -o $@ $^ $(LDFLAGS)

$(LIB_COMMON_S): \
	$(OBJ_COMMON)
	ar rcs $(LIB_COMMON_S) $^

# common-sdl

CFLAGS_SDL=$(shell sdl2-config --cflags)
LDFLAGS_SDL=$(shell sdl2-config --libs)

examples/common-sdl.o: \
	examples/common-sdl.cpp \
	examples/common-sdl.h
	$(CXX) $(CXXFLAGS) $(CFLAGS_SDL) -c $< -o $@

$(LIB_COMMON_SDL): \
	$(OBJ_SDL)
	$(CXX) $(CXXFLAGS) -shared -fPIC -o $@ $^ $(LDFLAGS) $(LDFLAGS_SDL)

$(LIB_COMMON_SDL_S): \
	$(OBJ_SDL)
	ar rcs $(LIB_COMMON_SDL_S) $^

clean:
	rm -vrf *.dot $(BUILD_TARGETS) $(TEST_TARGETS)
	rm -rvf src/*.o
	rm -rvf src/coreml/*.o
	rm -rvf tests/*.o
	rm -rvf examples/*.o
	rm -rvf *.a
	rm -rvf *.dll
	rm -rvf *.so
	rm -rvf *.dot
	rm -rvf ggml/*.a
	rm -rvf ggml/*.dll
	rm -rvf ggml/*.so
	rm -vrf ggml/src/*.o
	rm -vrf ggml/src/ggml-metal-embed.metal
	rm -vrf ggml/src/ggml-cuda/*.o
	rm -vrf ggml/src/ggml-cuda/template-instances/*.o
	rm -rvf $(BUILD_TARGETS)
	rm -rvf $(TEST_TARGETS)
	find examples -type f -name "*.o" -delete

#
# Examples
#

# $< is the first prerequisite, i.e. the source file.
# Explicitly compile this to an object file so that it can be cached with ccache.
# The source file is then filtered out from $^ (the list of all prerequisites) and the object file is added instead.

# Helper function that replaces .c, .cpp, and .cu file endings with .o:
GET_OBJ_FILE = $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(patsubst %.cu,%.o,$(1))))

main: examples/main/main.cpp \
	$(OBJ_GGML) $(OBJ_WHISPER) $(OBJ_COMMON)
	$(CXX) $(CXXFLAGS) -c $< -o $(call GET_OBJ_FILE, $<)
	$(CXX) $(CXXFLAGS) $(filter-out %.h $<,$^) $(call GET_OBJ_FILE, $<) -o $@ $(LDFLAGS)

bench: examples/bench/bench.cpp \
	$(OBJ_GGML) $(OBJ_WHISPER) $(OBJ_COMMON)
	$(CXX) $(CXXFLAGS) -c $< -o $(call GET_OBJ_FILE, $<)
	$(CXX) $(CXXFLAGS) $(filter-out %.h $<,$^) $(call GET_OBJ_FILE, $<) -o $@ $(LDFLAGS)

quantize: examples/quantize/quantize.cpp \
	$(OBJ_GGML) $(OBJ_WHISPER) $(OBJ_COMMON)
	$(CXX) $(CXXFLAGS) -c $< -o $(call GET_OBJ_FILE, $<)
	$(CXX) $(CXXFLAGS) $(filter-out %.h $<,$^) $(call GET_OBJ_FILE, $<) -o $@ $(LDFLAGS)

server: examples/server/server.cpp \
	$(OBJ_GGML) $(OBJ_WHISPER) $(OBJ_COMMON)
	$(CXX) $(CXXFLAGS) -c $< -o $(call GET_OBJ_FILE, $<)
	$(CXX) $(CXXFLAGS) $(filter-out %.h $<,$^) $(call GET_OBJ_FILE, $<) -o $@ $(LDFLAGS) $(LWINSOCK2)

command: examples/command/command.cpp \
	$(OBJ_GGML) $(OBJ_WHISPER) $(OBJ_COMMON) $(OBJ_SDL)
	$(CXX) $(CXXFLAGS) $(CFLAGS_SDL) -c $< -o $(call GET_OBJ_FILE, $<)
	$(CXX) $(CXXFLAGS) $(filter-out %.h $<,$^) $(call GET_OBJ_FILE, $<) -o $@ $(LDFLAGS) $(LDFLAGS_SDL)

stream: examples/stream/stream.cpp \
	$(OBJ_GGML) $(OBJ_WHISPER) $(OBJ_COMMON) $(OBJ_SDL)
	$(CXX) $(CXXFLAGS) $(CFLAGS_SDL) -c $< -o $(call GET_OBJ_FILE, $<)
	$(CXX) $(CXXFLAGS) $(filter-out %.h $<,$^) $(call GET_OBJ_FILE, $<) -o $@ $(LDFLAGS) $(LDFLAGS_SDL)

lsp: examples/lsp/lsp.cpp \
	$(OBJ_GGML) $(OBJ_WHISPER) $(OBJ_COMMON) $(OBJ_SDL)
	$(CXX) $(CXXFLAGS) $(CFLAGS_SDL) -c $< -o $(call GET_OBJ_FILE, $<)
	$(CXX) $(CXXFLAGS) $(filter-out %.h $<,$^) $(call GET_OBJ_FILE, $<) -o $@ $(LDFLAGS) $(LDFLAGS_SDL)

# TODO: disabled until update
#       https://github.com/ggerganov/whisper.cpp/issues/1818
#talk: examples/talk/talk.cpp examples/talk/gpt-2.cpp \
#	$(OBJ_GGML) $(OBJ_WHISPER) $(OBJ_COMMON) $(OBJ_SDL)
#	$(CXX) $(CXXFLAGS) $(CFLAGS_SDL) -c $< -o $(call GET_OBJ_FILE, $<)
#	$(CXX) $(CXXFLAGS) $(filter-out %.h $<,$^) $(call GET_OBJ_FILE, $<) -o $@ $(LDFLAGS) $(LDFLAGS_SDL)

talk-llama: examples/talk-llama/talk-llama.cpp examples/talk-llama/llama.cpp examples/talk-llama/llama-vocab.cpp examples/talk-llama/llama-grammar.cpp examples/talk-llama/llama-sampling.cpp examples/talk-llama/unicode.cpp examples/talk-llama/unicode-data.cpp \
	$(OBJ_GGML) $(OBJ_WHISPER) $(OBJ_COMMON) $(OBJ_SDL)
	$(CXX) $(CXXFLAGS) $(CFLAGS_SDL) -c $< -o $(call GET_OBJ_FILE, $<)
	$(CXX) $(CXXFLAGS) $(filter-out %.h $<,$^) $(call GET_OBJ_FILE, $<) -o $@ $(LDFLAGS) $(LDFLAGS_SDL)

#
# Tests
#

tests: $(TEST_TARGETS)

tests/test-c.o: tests/test-c.c include/whisper.h
	$(CC) $(CFLAGS) -c $(filter-out %.h,$^) -o $@

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
.PHONY: large-v3-turbo

tiny.en tiny base.en base small.en small medium.en medium large-v1 large-v2 large-v3 large-v3-turbo: main
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
