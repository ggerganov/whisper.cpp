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
- CPU is your CPU model
- OS is your current operating system
- Model is the GGML model being benchmarked
- Threads is the number of threads used
- Load is the time your computer took to load the model
- Encode is the time it took to run the Whisper encoder
- Time is reported as (real / process):
  real:    This is the wall-clock time in ms
  process: This the CPU time. If you're using multiple threads, the time spent in each thread will be added together.
           The proces time should be approximately (Proc Enc. / Threads).
           If it isn't, you likely have another program making use of the CPU
- Commit is the current git commit.

EOF

printf "|   CPU   |   OS   |      Config      |  Model  | Th | Load  | Encode        |  Commit  |\n"
printf "| ------- | ------ | ---------------- | ------- | -- | ----- | ------------- | -------- |\n"

for model in "${models[@]}"; do
    # run once to heat-up the cache
    ./bench -m "./models/ggml-$model.bin" -t "$n_threads" 2>/dev/null 1>/dev/null

    # actual run
    # store stderr output in a variable in order to parse it later
    output=$(./bench -m "./models/ggml-$model.bin" -t "$n_threads" 2>&1)

    # parse the output:
    load_proc=$(echo   "$output" | grep "load time"   | awk '{print $8}')
    load_real=$(echo   "$output" | grep "load time"   | awk '{print $5}')
    encode_proc=$(echo "$output" | grep "encode time" | awk '{print $8}')
    encode_real=$(echo "$output" | grep "encode time" | awk '{print $5}')
    system_info=$(echo "$output" | grep "system_info")
    n_threads=$(echo   "$output" | grep "system_info" | awk '{print $4}')

    # floor to milliseconds
    load_proc=${load_proc%.*}
    load_real=${load_real%.*}
    encode_proc=${encode_proc%.*}
    encode_real=${encode_real%.*}

    load_str="$load_real"
    encode_str="$encode_real / $encode_proc"

    config=$(echo "$system_info" | sed 's/ | /\n/g' | tail -n +2 | awk '/ = 1/{print $1}' | tr '\n' ' ')

    commit=$(git rev-parse --short HEAD)

    printf "| <todo>  | <todo> | %-16s | %-7s | %-2s | %-5s | %-13s | %-8s |\n" \
        "$config" \
        "$model" \
        "$n_threads" \
        "$load_str" \
        "$encode_str" \
        "$commit"
done

