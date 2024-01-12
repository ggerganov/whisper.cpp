#!/bin/sh

# This script downloads Whisper model files that have already been converted to ggml format.
# This way you don't have to convert them yourself.

#src="https://ggml.ggerganov.com"
#pfx="ggml-model-whisper"

src="https://huggingface.co/ggerganov/whisper.cpp"
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

models_path="${2:-$(get_script_path)}"

# Whisper models
models="tiny.en
tiny
tiny-q5_1
tiny.en-q5_1
base.en
base
base-q5_1
base.en-q5_1
small.en
small.en-tdrz
small
small-q5_1
small.en-q5_1
medium
medium.en
medium-q5_0
medium.en-q5_0
large-v1
large-v2
large-v3
large-v3-q5_0"

# list available models
list_models() {
    printf "\n"
    printf "  Available models:"
    for model in $models; do
        printf " %s" "$model"
    done
    printf "\n\n"
}

if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
    printf "Usage: %s <model> [models_path]\n" "$0"
    list_models

    exit 1
fi

model=$1

if ! echo "$models" | grep -q -w "$model"; then
    printf "Invalid model: %s\n" "$model"
    list_models

    exit 1
fi

# check if model contains `tdrz` and update the src and pfx accordingly
if echo "$model" | grep -q "tdrz"; then
    src="https://huggingface.co/akashmjn/tinydiarize-whisper.cpp"
    pfx="resolve/main/ggml"
fi

echo "$model" | grep -q '^"tdrz"*$'

# download ggml model

printf "Downloading ggml model %s from '%s' ...\n" "$model" "$src"

cd "$models_path" || exit

if [ -f "ggml-$model.bin" ]; then
    printf "Model %s already exists. Skipping download.\n" "$model"
    exit 0
fi

if [ -x "$(command -v wget)" ]; then
    wget --no-config --quiet --show-progress -O ggml-"$model".bin $src/$pfx-"$model".bin
elif [ -x "$(command -v curl)" ]; then
    curl -L --output ggml-"$model".bin $src/$pfx-"$model".bin
else
    printf "Either wget or curl is required to download models.\n"
    exit 1
fi


if [ $? -ne 0 ]; then
    printf "Failed to download ggml model %s \n" "$model"
    printf "Please try again later or download the original Whisper model files and convert them yourself.\n"
    exit 1
fi


printf "Done! Model '%s' saved in '%s/ggml-%s.bin'\n" "$model" "$models_path" "$model"
printf "You can now use it like this:\n\n"
printf "  $ ./main -m %s/ggml-%s.bin -f samples/jfk.wav\n" "$models_path" "$model"
printf "\n"
