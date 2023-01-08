#!/bin/bash

# Helper script to run the bench tool on all models and print the results in share-able format

printf "Usage: ./bench.sh [n_threads]\n"

if [ -z "$1" ]; then
    n_threads=4
else
    n_threads=$1
fi

models=( "tiny" "base" "small" "medium" "large" )

printf "\n"
printf "Running benchmark for all models\n"
printf "This can take a while!\n"
printf "\n"

cat >&2 << EOF
How to interpret these results:
- CPU is your CPU model.
- OS is your current operating system.
- Model is the GGML model being benchmarked.
- Threads is the number of threads used.
- Total Load is the total amount of CPU time whisper took to load the model.
- Real Load is the real amount of time your computer took to load the model.
- Total Encode is the total amount of CPU time whisper took to encode the input.
  If you're using multiple threads, the time spent in each thread will be added together.
- Real Encode is the real amount of time your computer took to encode the input.
  This should be approximately (Total Encode / Threads). If it isn't, you likely have another program making use of the CPU.
- Commit is the current git commit.

EOF

printf "|   CPU   |   OS   |      Config      |  Model  | Threads | Total Load | Real Load | Total Encode | Real Encode |  Commit  |\n"
printf "| ------- | ------ | ---------------- | ------- | ------- | ---------- | --------- | ------------ | ----------- | -------- |\n"

for model in "${models[@]}"; do
    # run once to heat-up the cache
    ./bench -m "./models/ggml-$model.bin" -t "$n_threads" 2>/dev/null 1>/dev/null

    # actual run
    # store stderr output in a variable in order to parse it later
    output=$(./bench -m "./models/ggml-$model.bin" -t "$n_threads" 2>&1)

    # parse the output:
    total_load_time=$(echo "$output" | grep "load time" | awk '{print $5}')
    real_load_time=$(echo "$output" | grep "load time" | awk '{print $8}')
    total_encode_time=$(echo "$output" | grep "encode time" | awk '{print $5}')
    real_encode_time=$(echo "$output" | grep "encode time" | awk '{print $8}')
    system_info=$(echo "$output" | grep "system_info")
    n_threads=$(echo "$output" | grep "system_info" | awk '{print $4}')

    # floor to milliseconds
    total_load_time=${total_load_time%.*}
    real_load_time=${real_load_time%.*}
    total_encode_time=${total_encode_time%.*}
    real_encode_time=${real_encode_time%.*}

    config=$(echo "$system_info" | sed 's/ | /\n/g' | tail -n +2 | awk '/ = 1/{print $1}' | tr '\n' ' ')

    commit=$(git rev-parse --short HEAD)

    printf "| <todo>  | <todo> | %-16s | %-7s | %-7s | %-10s | %-9s | %-12s | %-11s | %-8s |\n" \
        "$config" \
        "$model" \
        "$n_threads" \
        "$total_load_time" \
        "$real_load_time" \
        "$total_encode_time" \
        "$real_encode_time" \
        "$commit"
done

