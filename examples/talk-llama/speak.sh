#!/bin/bash

# Usage:
#  speak.sh <voice_id> <text-to-speak>

# espeak
# Mac OS: brew install espeak
# Linux: apt-get install espeak
#
#espeak -v en-us+m$1 -s 225 -p 50 -a 200 -g 5 -k 5 "$2"

# for Mac
if [ "$1" = "0" ]; then
    say "$2"
elif [ "$1" = "1" ]; then
    say -v "Samantha (Enhanced)" "$2"
elif [ "$1" = "2" ]; then
    say -v "Daniel (Enhanced)" "$2"
elif [ "$1" = "3" ]; then
    say -v "Veena (Enhanced)" "$2"
fi

# Eleven Labs
#
#wd=$(dirname $0)
#script=$wd/eleven-labs.py
#python3 $script $1 "$2" >/dev/null 2>&1
#ffplay -autoexit -nodisp -loglevel quiet -hide_banner -i ./audio.mp3 >/dev/null 2>&1
