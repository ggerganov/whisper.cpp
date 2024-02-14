import sys
import argparse

parser = argparse.ArgumentParser(description="Generate the TTS")
parser.add_argument("inputfile")
parser.add_argument("-v", "--voice", type=int, default=21,
    help="Get a voice object by number")
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

from elevenlabs import voices, generate, play, save, DEFAULT_VOICE

with open(args.inputfile) as f:
    voicelist = [DEFAULT_VOICE]
    voicelist += voices()[:]
    audio = generate(
        text=str(f.read()),
        voice=voicelist[args.voice % len(voicelist)]
    )
    if args.play:
        play(audio)
    else:
        save(audio, args.savefile) 
