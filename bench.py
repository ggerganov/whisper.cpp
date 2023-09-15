import subprocess
import re
import csv

# Define the models, threads, and processor counts to benchmark
models = [
    "ggml-tiny.en.bin",
    "ggml-base.en.bin",
    "ggml-small.en.bin",
    "ggml-medium.bin",
    "ggml-medium.en.bin",
    "ggml-large.bin",
    # "ggml-small.en-q5_1.bin",
    # "ggml-medium.en-q5_0.bin",
    # "ggml-large-q5_0.bin",
]

threads = [4]
processor_counts = [1]

# Initialize a dictionary to hold the results
results = {}


def extract_metrics(output: str, label: str) -> tuple[float, float]:
    match = re.search(rf"{label} \s*=\s*(\d+\.\d+)\s*ms\s*/\s*(\d+)\s*runs", output)
    time = float(match.group(1)) if match else None
    runs = float(match.group(2)) if match else None
    return time, runs


# Loop over each combination of parameters
for model in models:
    for thread in threads:
        for processor_count in processor_counts:
            print(f"{model} threads={thread} processor_count={processor_count}")
            # Construct the command to run
            cmd = f"./main -m models/{model} -t {thread} -p {processor_count} -f samples/alice-40s.wav"
            # Run the command and get the output
            process = subprocess.Popen(
                cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT
            )

            output = ""
            while process.poll() is None:
                output += process.stdout.read().decode()

            # Parse the output
            load_time_match = re.search(r"load time\s*=\s*(\d+\.\d+)\s*ms", output)
            load_time = float(load_time_match.group(1)) if load_time_match else None

            sample_time, sample_runs = extract_metrics(output, "sample time")
            encode_time, encode_runs = extract_metrics(output, "encode time")
            decode_time, decode_runs = extract_metrics(output, "decode time")

            total_time_match = re.search(r"total time\s*=\s*(\d+\.\d+)\s*ms", output)
            total_time = float(total_time_match.group(1)) if total_time_match else None
            # Store the times in the results dictionary
            results[(model, thread, processor_count)] = {
                "Load Time": load_time,
                "Sample Time": sample_time,
                "Encode Time": encode_time,
                "Decode Time": decode_time,
                "Sample Time per Run": sample_time / sample_runs,
                "Encode Time per Run": encode_time / encode_runs,
                "Decode Time per Run": decode_time / decode_runs,
                "Total Time": total_time,
            }

# Write the results to a CSV file
with open("benchmark_results.csv", "w", newline="") as csvfile:
    fieldnames = [
        "Model",
        "Thread",
        "Processor Count",
        "Load Time",
        "Sample Time",
        "Encode Time",
        "Decode Time",
        "Sample Time per Run",
        "Encode Time per Run",
        "Decode Time per Run",
        "Total Time",
    ]
    writer = csv.DictWriter(csvfile, fieldnames=fieldnames)

    writer.writeheader()
    # Sort the results by total time in ascending order
    sorted_results = sorted(results.items(), key=lambda x: x[1].get("Total Time", 0))
    for params, times in sorted_results:
        row = {
            "Model": params[0],
            "Thread": params[1],
            "Processor Count": params[2],
        }
        row.update(times)
        writer.writerow(row)
