import os
import subprocess
import re
import csv
import wave
import contextlib
import argparse
import json

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
    default="./6097_5_mins/",
    help="Relative path of the file to transcribe (default: ./samples/jfk.wav)",
)

parser.add_argument(
    "-s",
    "--type_set", 
    type=str, 
    default="./6097_5_mins/manifest.json", 
    help="Running WER set based on the validation / test set from the Commands Dataset\nSet path for the dataset"
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

validating_files = args.type_set

sample_folder = args.filename

threads = args.threads
processors = args.processors


def check_folder_exists(file: str) -> bool:
    return os.path.isdir(file)

def check_file_exists(file):
    return os.path.isfile(file)


if not check_folder_exists(sample_folder):
    raise FileNotFoundError(f"Sample file {sample_folder} not found")

filtered_models = []
for model in models:
    if check_file_exists(f"./models/{model}"):
        filtered_models.append(model)
    else:
        print(f"Model {model} not found, removing from list")

def filtered_text(output):
    pattern = re.compile(r'\[\d{2}:\d{2}:\d{2}\.\d{3} --> \d{2}:\d{2}:\d{2}\.\d{3}\]\s+(.*)')
    match = pattern.findall(output)
    return match

models = filtered_models

# read the valdiation list
manifest_data = []
with open(validating_files, 'r') as file:
    for line in file: 
        manifest_data.append(json.loads(line))

def calculate_wer(text, origin_word):
    ref_words = origin_word.split()
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



avg_size = len(manifest_data)
total_wer = 0
for model in filtered_models:
    for thread in threads:
        for processor_count in processors:
            for file in manifest_data: # we are running each iteration of the manifestation data
                audio_filepath = file['audio_filepath']
                audio_text = file['text']
                sample_file_path = sample_folder + audio_filepath
               # print(sample_file_path)
                cmd = f"./build/bin/whisper-cli -m models/{model} -t {thread} -p {processor_count} -f {sample_file_path}"
                process = subprocess.Popen(
                    cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT
                )

                output = ""
                while process.poll() is None:
                    output += process.stdout.read().decode()
                final_word = filtered_text(output)
                if len(final_word) == 0:
                    print(f"wer for {audio_filepath} is 1")
                    continue
                wer = calculate_wer(final_word[0], audio_text)
                print(f"wer for {audio_filepath} is {round(wer,2)}")
                total_wer += wer

print(f"Final WER is {round(total_wer / avg_size, 2)}")
                


