from elevenlabs.client import ElevenLabs
from elevenlabs import stream
import sys
import warnings


warnings.filterwarnings("ignore", message="Valid config keys have changed in V2:")
warnings.filterwarnings("ignore", message="Field \"model_id\" has conflict with protected namespace \"model_\".")


client = ElevenLabs(
      api_key=""
)

def text_stream(file_path):
    with open(file_path, 'r') as file:
        for line in file:
            yield line.strip()


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python eleven-speech.py <voice_id> <textfile>")
        sys.exit(1)

    voice_id = sys.argv[1]
    text_file = sys.argv[2]

    audio_stream = client.generate(
        text=text_stream(text_file),
        voice="Sexy", 
        model="eleven_turbo_v2_5",
        stream=True
    )

    stream(audio_stream)