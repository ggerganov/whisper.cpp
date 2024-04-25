#!/bin/sh

printf "whisper.cpp: this script hasn't been maintained and is not functional atm\n"
exit 1

# This script downloads Whisper model files that have already been converted to Core ML format.
# This way you don't have to convert them yourself.

src="https://huggingface.co/datasets/ggerganov/whisper.cpp-coreml"
pfx="resolve/main/ggml"

# get the path of this script
get_script_path() {
    if [ -x "$(command -v realpath)" ]; then
       dirname "$(realpath "$0")"
    else
        _ret="$(cd -- "$(dirname "$0")" >/dev/null 2>&1 || exit ; pwd -P)"
        echo "$_ret"
    fi
}

models_path="$(get_script_path)"

# Whisper models
models="tiny.en tiny base.en base small.en small medium.en medium large-v1 large-v2 large-v3"

# list available models
list_models() {
        printf "\n"
        printf "  Available models:"
        for model in $models; do
                printf " %s" "$models"
        done
        printf "\n\n"
}

if [ "$#" -ne 1 ]; then
    printf "Usage: %s <model>\n" "$0"
    list_models

    exit 1
fi

model=$1

if ! echo "$models" | grep -q -w "$model"; then
    printf "Invalid model: %s\n" "$model"
    list_models

    exit 1
fi

# download Core ML model

printf "Downloading Core ML model %s from '%s' ...\n" "$model" "$src"

cd "$models_path" || exit

if [ -f "ggml-$model.mlmodel" ]; then
    printf "Model %s already exists. Skipping download.\n" "$model"
    exit 0
fi

if [ -x "$(command -v wget)" ]; then
    wget --quiet --show-progress -O ggml-"$model".mlmodel $src/$pfx-"$model".mlmodel
elif [ -x "$(command -v curl)" ]; then
    curl -L --output ggml-"$model".mlmodel $src/$pfx-"$model".mlmodel
else
    printf "Either wget or curl is required to download models.\n"
    exit 1
fi


if [ $? -ne 0 ]; then
    printf "Failed to download Core ML model %s \n" "$model"
    printf "Please try again later or download the original Whisper model files and convert them yourself.\n"
    exit 1
fi

printf "Done! Model '%s' saved in 'models/ggml-%s.mlmodel'\n" "$model" "$model"
printf "Run the following command to compile it:\n\n"
printf "  $ xcrun coremlc compile ./models/ggml-%s.mlmodel ./models\n\n" "$model"
printf "You can now use it like this:\n\n"
printf "  $ ./main -m models/ggml-%s.bin -f samples/jfk.wav\n" "$model"
printf "\n"
