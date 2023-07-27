#!/bin/bash

# Usage:
#  speak.sh <voice_id> <text-to-speak>

# espeak
# Mac OS: brew install espeak
# Linux: apt-get install espeak
#
#espeak -v en-us+m$1 -s 225 -p 50 -a 200 -g 5 -k 5 "$2"

# for Mac
say "$2"

# Eleven Labs
# To use it, install the elevenlabs module from pip (pip install elevenlabs)
# It's possible to use the API for free with limited number of characters. To increase this limit register to https://beta.elevenlabs.io to get an api key and paste it after 'ELEVEN_API_KEY='
#Keep the line commented to use the free version whitout api key
#
#export ELEVEN_API_KEY=your_api_key
#wd=$(dirname $0)
#script=$wd/eleven-labs.py
#python3 $script $1 "$2" >/dev/null 2>&1
#ffplay -autoexit -nodisp -loglevel quiet -hide_banner -i ./audio.mp3 >/dev/null 2>&1
