#!/bin/bash

models=( "tiny.en" "tiny" "base.en" "base" "small.en" "small" "medium.en" "medium" "large-v1" "large" )

for model in "${models[@]}"; do
    python3 models/convert-pt-to-ggml.py ~/.cache/whisper/$model.pt ../whisper models/
    mv -v models/ggml-model.bin models/ggml-$model.bin
done
