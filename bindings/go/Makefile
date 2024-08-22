ifndef UNAME_S
UNAME_S := $(shell uname -s)
endif

ifndef UNAME_P
UNAME_P := $(shell uname -p)
endif

ifndef UNAME_M
UNAME_M := $(shell uname -m)
endif

GGML_METAL_PATH_RESOURCES := $(abspath ../../ggml/src)
BUILD_DIR := build
MODELS_DIR := models
EXAMPLES_DIR := $(wildcard examples/*)
INCLUDE_PATH := $(abspath ../../include):$(abspath ../../ggml/include)
LIBRARY_PATH := $(abspath ../..):$(abspath ../../src):$(abspath ../../ggml/src)

ifeq ($(UNAME_S),Darwin)
	EXT_LDFLAGS := -framework Foundation -framework Metal -framework MetalKit
endif

all: clean whisper examples

whisper: mkdir
	@echo Build whisper
	@${MAKE} -C ../.. libwhisper.a

test: model-small whisper modtidy
ifeq ($(UNAME_S),Darwin)
	@C_INCLUDE_PATH=${INCLUDE_PATH} LIBRARY_PATH=${LIBRARY_PATH} GGML_METAL_PATH_RESOURCES=${GGML_METAL_PATH_RESOURCES} go test -ldflags "-extldflags '$(EXT_LDFLAGS)'" -v .
	@C_INCLUDE_PATH=${INCLUDE_PATH} LIBRARY_PATH=${LIBRARY_PATH} GGML_METAL_PATH_RESOURCES=${GGML_METAL_PATH_RESOURCES} go test -ldflags "-extldflags '$(EXT_LDFLAGS)'" -v ./pkg/whisper/...
else
	@C_INCLUDE_PATH=${INCLUDE_PATH} LIBRARY_PATH=${LIBRARY_PATH} go test -v .
	@C_INCLUDE_PATH=${INCLUDE_PATH} LIBRARY_PATH=${LIBRARY_PATH} go test -v ./pkg/whisper/...
endif

examples: $(EXAMPLES_DIR)

model-small: mkdir examples/go-model-download
	@${BUILD_DIR}/go-model-download -out models ggml-small.en.bin

$(EXAMPLES_DIR): mkdir whisper modtidy
	@echo Build example $(notdir $@)
ifeq ($(UNAME_S),Darwin)
	@C_INCLUDE_PATH=${INCLUDE_PATH} LIBRARY_PATH=${LIBRARY_PATH} GGML_METAL_PATH_RESOURCES=${GGML_METAL_PATH_RESOURCES} go build ${BUILD_FLAGS} -ldflags "-extldflags '$(EXT_LDFLAGS)'" -o ${BUILD_DIR}/$(notdir $@) ./$@
else
	@C_INCLUDE_PATH=${INCLUDE_PATH} LIBRARY_PATH=${LIBRARY_PATH} go build ${BUILD_FLAGS} -o ${BUILD_DIR}/$(notdir $@) ./$@
endif

mkdir:
	@echo Mkdir ${BUILD_DIR}
	@install -d ${BUILD_DIR}
	@echo Mkdir ${MODELS_DIR}
	@install -d ${MODELS_DIR}

modtidy:
	@go mod tidy

clean:
	@echo Clean
	@rm -fr $(BUILD_DIR)
	@go clean
