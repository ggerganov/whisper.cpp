#!/bin/bash
set -eo pipefail
# Transcribe audio livestream by feeding ffmpeg output to whisper.cpp at regular intervals
# Idea by @semiformal-net
# ref: https://github.com/ggerganov/whisper.cpp/issues/185
#
# TODO:
# - Currently, there is a gap between sequential chunks, so some of the words are dropped. Need to figure out a
#   way to produce a continuous stream of audio chunks.
#

url="http://a.files.bbci.co.uk/media/live/manifesto/audio/simulcast/hls/nonuk/sbr_low/ak/bbc_world_service.m3u8"
fmt=aac # the audio format extension of the stream (TODO: auto detect)
step_s=30
model="base.en"

if [ -z "$1" ]; then
    echo "Usage: $0 stream_url [step_s] [model]"
    echo ""
    echo "  Example:"
    echo "    $0 $url $step_s $model"
    echo ""
    echo "No url specified, using default: $url"
else
    url="$1"
fi

if [ -n "$2" ]; then
    step_s="$2"
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

#trap "running=0" SIGINT SIGTERM

printf "[+] Transcribing stream with model '$model', step_s $step_s (press Ctrl+C to stop):\n\n"

# continuous stream in native fmt (this file will grow forever!)
ffmpeg -loglevel quiet -y -re -probesize 32 -i $url -c copy /tmp/whisper-live0.${fmt}  &
if [ $? -ne 0 ]; then
    printf "Error: ffmpeg failed to capture audio stream\n"
    exit 1
fi
printf "Buffering audio. Please wait...\n"
# For some reason, the initial buffer can end up smaller than step_s (even though we sleep for step_s)
sleep $(($step_s*2))
i=0
while [ $running -eq 1 ]; do
    # a handy bash built-in, SECONDS,
    # > "This variable expands to the number of seconds since the shell was started. Assignment to this variable resets the count to the value assigned, and the expanded value becomes the value assigned
    # > plus the number of seconds since the assignment."
    SECONDS=0
    # extract the next piece from the main file above and transcode to wav. -ss sets start time and nudges it by -0.5s to catch missing words (??)
    if [ $i -gt 0 ]; then
        ffmpeg -loglevel quiet -noaccurate_seek -i /tmp/whisper-live0.${fmt} -y -ar 16000 -ac 1 -c:a pcm_s16le -ss $(($i*$step_s-1)).5 -t $step_s /tmp/whisper-live.wav
    else
        ffmpeg -loglevel quiet -noaccurate_seek -i /tmp/whisper-live0.${fmt} -y -ar 16000 -ac 1 -c:a pcm_s16le -ss $(($i*$step_s)) -t $step_s /tmp/whisper-live.wav
    fi
    ./main -t 8 -m ./models/ggml-base.en.bin -f /tmp/whisper-live.wav --no-timestamps -otxt 2> /tmp/whispererr | tail -n 1
    echo
    while [ $SECONDS -lt $step_s ]; do
        sleep 1
    done
    ((i=i+1))
done
