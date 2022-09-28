#!/bin/bash

# This script downloads Whisper model files that have already been converted to ggml format.
# This way you don't have to convert them yourself.

ggml_path=$(dirname $(realpath $0))

# Whisper models
models=( "tiny.en" "tiny" "base.en" "base" "small.en" "small" "medium.en" "medium" "large" )

# list available models
function list_models {
    printf "\n"
    printf "  Available models:"
    for model in "${models[@]}"; do
        printf " $model"
    done
    printf "\n\n"
}

if [ "$#" -ne 1 ]; then
    printf "Usage: $0 <model>\n"
    list_models

    exit 1
fi

model=$1

if [[ ! " ${models[@]} " =~ " ${model} " ]]; then
    printf "Invalid model: $model\n"
    list_models

    exit 1
fi

# download ggml model

printf "Downloading ggml model $model ...\n"

mkdir -p models

if [ -f "models/ggml-$model.bin" ]; then
    printf "Model $model already exists. Skipping download.\n"
    exit 0
fi

wget --quiet --show-progress -O models/ggml-$model.bin https://ggml.ggerganov.com/ggml-model-whisper-$model.bin

if [ $? -ne 0 ]; then
    printf "Failed to download ggml model $model \n"
    printf "Please try again later or download the original Whisper model files and convert them yourself.\n"
    exit 1
fi

printf "Done! Model '$model' saved in 'models/ggml-$model.bin'\n"
printf "You can now use it like this:\n\n"
printf "  $ ./main -m models/ggml-$model.bin -f samples/jfk.wav\n"
printf "\n"
