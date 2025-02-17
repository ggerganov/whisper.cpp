from pydub import AudioSegment
import os

def adjust_audio_to_16khz(input_folder, output_folder):
    if not os.path.exists(output_folder):
        os.makedirs(output_folder)

    for root, _, files in os.walk(input_folder):
        for file in files:
            if file.endswith(('.wav', '.mp3', '.flac')):
                input_path = os.path.join(root, file)
                output_path = os.path.join(output_folder, file)

                audio = AudioSegment.from_file(input_path)
                audio = audio.set_frame_rate(16000)
                audio.export(output_path, format="wav")
                print(f"Adjusted {file} to 16 kHz")

input_folder = "./6097_5_mins/"
output_folder = "./6097_5_mins_16khz/"

adjust_audio_to_16khz(input_folder, output_folder)