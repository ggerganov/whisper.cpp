#!/bin/bash

printf "Usage: $0 <upload>"

if [ $# -ne 1 ]; then
    printf "\nError: Invalid number of arguments\n"
    exit 1
fi

qtype0="q5_0"
qtype1="q5_1"
upload="$1"

cd `dirname $0`
cd ../

./quantize ./models/ggml-tiny.en.bin   ./models/ggml-tiny.en-${qtype1}.bin ${qtype1}
./quantize ./models/ggml-tiny.bin      ./models/ggml-tiny-${qtype1}.bin    ${qtype1}

./quantize ./models/ggml-base.en.bin   ./models/ggml-base.en-${qtype1}.bin ${qtype1}
./quantize ./models/ggml-base.bin      ./models/ggml-base-${qtype1}.bin    ${qtype1}

./quantize ./models/ggml-small.en.bin  ./models/ggml-small.en-${qtype1}.bin ${qtype1}
./quantize ./models/ggml-small.bin     ./models/ggml-small-${qtype1}.bin    ${qtype1}

./quantize ./models/ggml-medium.en.bin ./models/ggml-medium.en-${qtype0}.bin ${qtype0}
./quantize ./models/ggml-medium.bin    ./models/ggml-medium-${qtype0}.bin    ${qtype0}

./quantize ./models/ggml-large.bin     ./models/ggml-large-${qtype0}.bin ${qtype0}

if [ "$upload" == "1" ]; then
    scp ./models/ggml-tiny.en-${qtype1}.bin   root@linode0:/mnt/Data/ggml/ggml-model-whisper-tiny.en-${qtype1}.bin
    scp ./models/ggml-tiny-${qtype1}.bin      root@linode0:/mnt/Data/ggml/ggml-model-whisper-tiny-${qtype1}.bin

    scp ./models/ggml-base.en-${qtype1}.bin   root@linode0:/mnt/Data/ggml/ggml-model-whisper-base.en-${qtype1}.bin
    scp ./models/ggml-base-${qtype1}.bin      root@linode0:/mnt/Data/ggml/ggml-model-whisper-base-${qtype1}.bin

    scp ./models/ggml-small.en-${qtype1}.bin  root@linode0:/mnt/Data/ggml/ggml-model-whisper-small.en-${qtype1}.bin
    scp ./models/ggml-small-${qtype1}.bin     root@linode0:/mnt/Data/ggml/ggml-model-whisper-small-${qtype1}.bin

    scp ./models/ggml-medium.en-${qtype0}.bin root@linode0:/mnt/Data/ggml/ggml-model-whisper-medium.en-${qtype0}.bin
    scp ./models/ggml-medium-${qtype0}.bin    root@linode0:/mnt/Data/ggml/ggml-model-whisper-medium-${qtype0}.bin

    scp ./models/ggml-large-${qtype0}.bin     root@linode0:/mnt/Data/ggml/ggml-model-whisper-large-${qtype0}.bin
fi
