import sys
import importlib.util

api_key = "" #Write your https://beta.elevenlabs.io api key here
if not api_key:
    print("To use elevenlabs you have to register to https://beta.elevenlabs.io and add your elevenlabs api key to examples/talk-llama/eleven-labs.py")
    sys.exit()

if importlib.util.find_spec("elevenlabs") is None:
    print("elevenlabs library is not installed, you can install it to your enviroment using 'pip install elevenlabs'")
    sys.exit()

from elevenlabs import ElevenLabs
eleven = ElevenLabs(api_key)

# Get a Voice object, by name or UUID
voice = eleven.voices["Arnold"] #Possible Voices: Adam Antoni Arnold Bella Domi Elli Josh

# Generate the TTS
audio = voice.generate(str(sys.argv[2:]))

# Save the TTS to a file
audio.save("audio") 
