#!/bin/bash

# Usage:
#  speak.sh <voice_id> <text-to-speak>

# espeak
# Mac OS: brew install espeak
# Linux: apt-get install espeak
#
#espeak -v en-us+m$1 -s 175 -p 50 -a 200 -g 5 -k 5 "$2"

# Mac OS "say" command
say "$2"

# Eleven Labs
# To use it, install the elevenlabs module from pip (pip install elevenlabs), register to https://beta.elevenlabs.io to get an api key and paste it in /examples/talk/eleven-labs.py 
#
#wd=$(dirname $0)
#script=$wd/eleven-labs.py
#python3 $script $1 "$2"
#ffplay -autoexit -nodisp -loglevel quiet -hide_banner -i ./audio.mp3
