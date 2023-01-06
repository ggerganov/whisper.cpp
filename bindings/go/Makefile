BUILD_DIR := build
MODELS_DIR := models
EXAMPLES_DIR := $(wildcard examples/*)
INCLUDE_PATH := $(abspath ../..)
LIBRARY_PATH := $(abspath ../..)

all: clean whisper examples

whisper: mkdir
	@echo Build whisper
	@${MAKE} -C ../.. libwhisper.a

test: model-small whisper modtidy
	@C_INCLUDE_PATH=${INCLUDE_PATH} LIBRARY_PATH=${LIBRARY_PATH} go test -v .
	@C_INCLUDE_PATH=${INCLUDE_PATH} LIBRARY_PATH=${LIBRARY_PATH} go test -v ./pkg/whisper/...

examples: $(EXAMPLES_DIR)

model-small: mkdir examples/go-model-download
	@${BUILD_DIR}/go-model-download -out models ggml-small.en.bin

$(EXAMPLES_DIR): mkdir whisper modtidy
	@echo Build example $(notdir $@)
	@C_INCLUDE_PATH=${INCLUDE_PATH} LIBRARY_PATH=${LIBRARY_PATH} go build ${BUILD_FLAGS} -o ${BUILD_DIR}/$(notdir $@) ./$@

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
