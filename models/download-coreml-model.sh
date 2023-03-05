#!/bin/bash

# This script downloads Whisper model files that have already been converted to Core ML format.
# This way you don't have to convert them yourself.

src="https://huggingface.co/datasets/ggerganov/whisper.cpp-coreml"
pfx="resolve/main/ggml"

# get the path of this script
function get_script_path() {
    if [ -x "$(command -v realpath)" ]; then
        echo "$(dirname $(realpath $0))"
    else
        local ret="$(cd -- "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P)"
        echo "$ret"
    fi
}

models_path="$(get_script_path)"

# Whisper models
models=( "tiny.en" "tiny" "base.en" "base" "small.en" "small" "medium.en" "medium" "large-v1" "large" )

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

# download Core ML model

printf "Downloading Core ML model $model from '$src' ...\n"

cd $models_path

if [ -f "ggml-$model.mlmodel" ]; then
    printf "Model $model already exists. Skipping download.\n"
    exit 0
fi

if [ -x "$(command -v wget)" ]; then
    wget --quiet --show-progress -O ggml-$model.mlmodel $src/$pfx-$model.mlmodel
elif [ -x "$(command -v curl)" ]; then
    curl -L --output ggml-$model.mlmodel $src/$pfx-$model.mlmodel
else
    printf "Either wget or curl is required to download models.\n"
    exit 1
fi


if [ $? -ne 0 ]; then
    printf "Failed to download Core ML model $model \n"
    printf "Please try again later or download the original Whisper model files and convert them yourself.\n"
    exit 1
fi

printf "Done! Model '$model' saved in 'models/ggml-$model.mlmodel'\n"
printf "Run the following command to compile it:\n\n"
printf "  $ xcrun coremlc compile ./models/ggml-$model.mlmodel ./models\n\n"
printf "You can now use it like this:\n\n"
printf "  $ ./main -m models/ggml-$model.bin -f samples/jfk.wav\n"
printf "\n"
