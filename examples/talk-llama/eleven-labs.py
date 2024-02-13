import sys
import importlib.util
import argparse

if importlib.util.find_spec("elevenlabs") is None:
    print("elevenlabs library is not installed, you can install it to your enviroment using 'pip install elevenlabs'")
    sys.exit()

parser = argparse.ArgumentParser(description="Generate the TTS")
parser.add_argument("inputfile")
parser.add_argument("-v", "--voice", default="Arnold",
    choices=["Adam", "Antoni", "Arnold", "Bella", "Domi", "Elli", "Josh"],
    help="Get a voice object by name")
group = parser.add_mutually_exclusive_group()
group.add_argument("-s", "--savefile", default="audio.mp3",
    help="Save the TTS to a file")
group.add_argument("-p", "--play", action="store_true",
    help="Play the TTS with ffplay")
args = parser.parse_args()

from elevenlabs import generate, play, save

with open(args.inputfile) as f:
    audio = generate(
        text=str(f.read()),
        voice=args.voice
    )
    if args.play:
        play(audio)
    else:
        save(audio, args.savefile) 
