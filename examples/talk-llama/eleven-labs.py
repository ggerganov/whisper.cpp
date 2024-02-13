import sys
import argparse

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
parser.add_argument("-q", "--quiet", action="store_true",
    help="Quietly skip checking the required library")
args = parser.parse_args()

if not args.quiet:
    import importlib.util
    if importlib.util.find_spec("elevenlabs") is None:
        print("elevenlabs library is not installed, you can install it to your enviroment using 'pip install elevenlabs'")
        sys.exit()

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
