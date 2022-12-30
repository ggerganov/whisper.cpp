CMAKE := $(shell which cmake)
BUILD_DIR := "build"
MODELS_DIR := "models"
EXAMPLES_DIR := $(wildcard examples/*)
C_INCLUDE_PATH := "../.."

all: clean whisper examples

whisper: mkdir
	@echo Build whisper
	@${CMAKE} -S ../.. -B ${BUILD_DIR} -D BUILD_SHARED_LIBS=off -D WHISPER_NO_AVX2=on
	@${CMAKE} --build ${BUILD_DIR} --target whisper

test: model-small whisper modtidy
	@go test -v .
	@go test -v ./pkg/whisper/...

examples: $(EXAMPLES_DIR)

model-small: mkdir examples/go-model-download
	@${BUILD_DIR}/go-model-download -out models small.en

$(EXAMPLES_DIR): mkdir whisper modtidy
	@echo Build example $(notdir $@)
	@go build ${BUILD_FLAGS} -o ${BUILD_DIR}/$(notdir $@) ./$@

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
