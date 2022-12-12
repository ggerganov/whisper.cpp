#!/usr/bin/env bash

# Small shell script to more easily automatically download and transcribe live stream VODs.
# This uses YT-DLP, ffmpeg and the CPP version of Whisper: https://github.com/ggerganov/whisper.cpp
# Use `./examples/yt-wsp.sh help` to print help info.
#
# Sample usage:
#
#   git clone https://github.com/ggerganov/whisper.cpp
#   cd whisper.cpp
#   make
#   ./examples/yt-wsp.sh https://www.youtube.com/watch?v=1234567890
#

# MIT License

# Copyright (c) 2022 Daniils Petrovs

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

set -Eeuo pipefail

# You can find how to download models in the OG repo: https://github.com/ggerganov/whisper.cpp/#usage
MODEL_PATH="${MODEL_PATH:-models/ggml-base.en.bin}" # Set to a multilingual model if you want to translate from foreign lang to en
WHISPER_EXECUTABLE="${WHISPER_EXECUTABLE:-whisper}" # Where to find the whisper.cpp executable
WHISPER_LANG="${WHISPER_LANG:-en}" # Set to desired lang to translate from

msg() {
    echo >&2 -e "${1-}"
}

cleanup() {
    msg "Cleaning up..."
    rm -rf "${temp_dir}" "vod-resampled.wav" "vod-resampled.wav.srt"
}

print_help() {
    echo "Usage: ./examples/yt-wsp.sh <video_url>"
    echo "See configurable env variables in the script"
    echo "This will produce an MP4 muxed file called res.mp4 in the working directory"
    echo "Requirements: ffmpeg yt-dlp whisper"
    echo "Whisper needs to be built into the main binary with make, then you can rename it to something like 'whisper' and add it to your PATH for convenience."
    echo "E.g. in the root of Whisper.cpp, run: 'make && cp ./main /usr/local/bin/whisper'"
}

check_requirements() {
    if ! command -v ffmpeg &>/dev/null; then
        echo "ffmpeg is required (https://ffmpeg.org)."
        exit 1
    fi

    if ! command -v yt-dlp &>/dev/null; then
        echo "yt-dlp is required (https://github.com/yt-dlp/yt-dlp)."
        exit 1
    fi

    if ! command -v "$WHISPER_EXECUTABLE" &>/dev/null; then
        WHISPER_EXECUTABLE="./main"
        if ! command -v "$WHISPER_EXECUTABLE" &>/dev/null; then
            echo "Whisper is required (https://github.com/ggerganov/whisper.cpp):"
            echo "Sample usage:"
            echo ""
            echo "  git clone https://github.com/ggerganov/whisper.cpp"
            echo "  cd whisper.cpp"
            echo "  make"
            echo "  ./examples/yt-wsp.sh https://www.youtube.com/watch?v=1234567890"
            echo ""
            exit 1
        fi
    fi
}

if [[ $# -lt 1 ]]; then
    print_help
    exit 1
fi

if [[ "$1" == "help" ]]; then
    print_help
    exit 0
fi

temp_dir="tmp"
source_url="$1"

check_requirements

msg "Downloading VOD..."

# Optionally add --cookies-from-browser BROWSER[+KEYRING][:PROFILE][::CONTAINER] for members only VODs
yt-dlp \
    -f "bestvideo[ext=mp4]+bestaudio[ext=m4a]/best[ext=mp4]/best" \
    --embed-thumbnail \
    --embed-chapters \
    --xattrs \
    "${source_url}" -o "${temp_dir}/vod.mp4"

msg "Extracting audio and resampling..."

ffmpeg -i "${temp_dir}/vod.mp4" \
    -hide_banner \
    -loglevel error \
    -ar 16000 \
    -ac 1 \
    -c:a \
    pcm_s16le -y "vod-resampled.wav"

msg "Transcribing to subtitle file..."
msg "Whisper specified at: ${WHISPER_EXECUTABLE}"

$WHISPER_EXECUTABLE \
    -m "${MODEL_PATH}" \
    -l "${WHISPER_LANG}" \
    -f "vod-resampled.wav" \
    -t 8 \
    -osrt \
    --translate

msg "Embedding subtitle track..."

ffmpeg -i "${temp_dir}/vod.mp4" \
    -hide_banner \
    -loglevel error \
    -i "vod-resampled.wav.srt" \
    -c copy \
    -c:s mov_text \
    -y res.mp4

cleanup

msg "Done! Your finished file is ready: res.mp4"
