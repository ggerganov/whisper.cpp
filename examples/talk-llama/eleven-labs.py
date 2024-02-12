import sys
import importlib.util

if importlib.util.find_spec("elevenlabs") is None:
    print("elevenlabs library is not installed, you can install it to your enviroment using 'pip install elevenlabs'")
    sys.exit()

from elevenlabs import generate, play, save

# Get a Voice object, by name or UUID
voice = "Arnold" #Possible Voices: Adam Antoni Arnold Bella Domi Elli Josh

with open(sys.argv[1]) as f:
  # Generate the TTS
  audio = generate(
    text=str(f.read()),
    voice=voice
  )

  # Save the TTS to a file
  save(audio, "audio.mp3") 
