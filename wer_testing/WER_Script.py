import os
import subprocess
import re
import csv
import wave
import contextlib
import argparse


# Custom action to handle comma-separated list
class ListAction(argparse.Action):
    def __call__(self, parser, namespace, values, option_string=None):
        setattr(namespace, self.dest, [int(val) for val in values.split(",")])


parser = argparse.ArgumentParser(description="Benchmark the speech recognition model")

# Define the argument to accept a list
parser.add_argument(
    "-t",
    "--threads",
    dest="threads",
    action=ListAction,
    default=[4],
    help="List of thread counts to benchmark (comma-separated, default: 4)",
)

parser.add_argument(
    "-p",
    "--processors",
    dest="processors",
    action=ListAction,
    default=[1],
    help="List of processor counts to benchmark (comma-separated, default: 1)",
)


parser.add_argument(
    "-f",
    "--filename",
    type=str,
    default="./samples/jfk.wav",
    help="Relative path of the file to transcribe (default: ./samples/jfk.wav)",
)

parser.add_arguemnt(
    "-t",
    "--type_set", 
    type = str, 
    default = "./speech_commands_v0.01/validation_list.txt", 
    help=" Running WER set based on the validation / test set from the Commands Dataset \
        \n Set path for the dataset"
)

# Parse the command line arguments
args = parser.parse_args()



models = [
    "ggml-tiny.en.bin",
    "ggml-tiny.bin",
    "ggml-base.en.bin",
    "ggml-base.bin",
    "ggml-small.en.bin",
    "ggml-small.bin",
    "ggml-medium.en.bin",
    "ggml-medium.bin",
    "ggml-large-v1.bin",
    "ggml-large-v2.bin",
    "ggml-large-v3.bin",
    "ggml-large-v3-turbo.bin",
]

testing_files = "./speech_commands_v0.01/testing_list.txt"
validating_files = "./speech_commands_v0.01/validation_list.txt"


sample_file = args.filename

threads = args.threads
processors = args.processors


def check_file_exists(file: str) -> bool:
    return os.path.isfile(file)


if not check_file_exists(sample_file):
    raise FileNotFoundError(f"Sample file {sample_file} not found")

filtered_models = []
for model in models:
    if check_file_exists(f"./models/{model}"):
        filtered_models.append(model)
    else:
        print(f"Model {model} not found, removing from list")

def filterd_text(output):
    pattern = re.compile(r'\[\d{2}:\d{2}:\d{2}\.\d{3} --> \d{2}:\d{2}:\d{2}\.\d{3}\]\s+(.*)')
    match = pattern.findall(output)
    return match

def filtered_word(file_path):
    return re.match(r'([^/]+)/', file_path).group(1)

models = filtered_models

# read the valdiation list
with open(args.type_set, 'r') as file:
    validating_files = file.read().splitlines()

def stitch_audio(validation_files, output_file):
    word = []
    with open("file_list.txt", "w") as f:
        for sample_file in validation_files:
            sample_file_path = f"./speech_commands_v0.01/{sample_file}"
            print(sample_file_path)
            sample_word = filtered_word(sample_file)
            word.append(sample_word)
            if check_file_exists(sample_file_path):
                print(sample_file_path)
                f.write(f"file '{sample_file_path}'\n")
            else:
                print(f"Sample file {sample_file_path} not found, skipping")

    cmd = f"ffmpeg -f concat -safe 0 -i file_list.txt -c copy {output_file}"
    subprocess.run(cmd, shell=True)
    # Remove the temporary file list
    os.remove("file_list.txt")
    return word

audio_word = stitch_audio(validating_files, "output_stitched.wav")

sample_file_path = "output_stitched.wav"
def calculate_wer(text, audio_word):
    ref_words = audio_word
    hyp_words = text.split()

    if not ref_words and not hyp_words:
        return 0

    elif not ref_words:
        return float('inf') if hyp_words else 1
    elif not hyp_words:
        return float('inf')
    
   # Initialize the dynamic programming table (list of lists)
    d = [[0 for j in range(len(hyp_words) + 1)] for i in range(len(ref_words) + 1)]

    for i in range(len(ref_words) + 1):
        d[i][0] = i
    for j in range(len(hyp_words) + 1):
        d[0][j] = j

    for i in range(1, len(ref_words) + 1):
        for j in range(1, len(hyp_words) + 1):
            if ref_words[i - 1] == hyp_words[j - 1]:
                d[i][j] = d[i - 1][j - 1]
            else:
                d[i][j] = 1 + min(d[i - 1][j], d[i][j - 1], d[i - 1][j - 1])

    wer = d[len(ref_words)][len(hyp_words)] / len(ref_words)
    return wer


# if not check_file_exists(sample_file_path):
#     print(f"Sample file {sample_file} not found, skipping")

for model in filtered_models:
    for thread in threads:
        for processor_count in processors:
            cmd = f"./build/bin/whisper-cli -m models/{model} -t {thread} -p {processor_count} -f {sample_file_path}"
            process = subprocess.Popen(
                cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT
            )

            output = ""
            while process.poll() is None:
                output += process.stdout.read().decode()

            final_wer = calculate_wer(output, audio_word)
            print("FINAL WER IS:")
            print(final_wer)
            



