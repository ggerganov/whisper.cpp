#!/bin/bash

# Transcribe audio livestream by feeding ffmpeg output to whisper.cpp at regular intervals
# Idea by @semiformal-net
# ref: https://github.com/ggerganov/whisper.cpp/issues/185
#
# TODO:
# - Currently, there is a gap between sequential chunks, so some of the words are dropped. Need to figure out a
#   way to produce a continuous stream of audio chunks.
#

url="http://a.files.bbci.co.uk/media/live/manifesto/audio/simulcast/hls/nonuk/sbr_low/ak/bbc_world_service.m3u8"
step_ms=10000
model="base.en"

if [ -z "$1" ]; then
    echo "Usage: $0 stream_url [step_ms] [model]"
    echo ""
    echo "  Example:"
    echo "    $0 $url $step_ms $model"
    echo ""
    echo "No url specified, using default: $url"
else
    url="$1"
fi

if [ -n "$2" ]; then
    step_ms="$2"
fi

if [ -n "$3" ]; then
    model="$3"
fi

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

if [[ ! " ${models[@]} " =~ " ${model} " ]]; then
    printf "Invalid model: $model\n"
    list_models

    exit 1
fi

running=1

trap "running=0" SIGINT SIGTERM

printf "[+] Transcribing stream with model '$model', step_ms $step_ms (press Ctrl+C to stop):\n\n"

while [ $running -eq 1 ]; do
    ffmpeg -y -re -probesize 32 -i $url -ar 16000 -ac 1 -c:a pcm_s16le -t ${step_ms}ms /tmp/whisper-live0.wav > /dev/null 2> /tmp/whisper-live.err
    if [ $? -ne 0 ]; then
        printf "Error: ffmpeg failed to capture audio stream\n"
        exit 1
    fi
    mv /tmp/whisper-live0.wav /tmp/whisper-live.wav
    ./main -t 8 -m ./models/ggml-small.en.bin -f /tmp/whisper-live.wav --no-timestamps -otxt 2> /tmp/whispererr | tail -n 1 &
done
