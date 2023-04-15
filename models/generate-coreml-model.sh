#!/bin/bash

# Usage: ./generate-coreml-model.sh <model-name>
if [ $# -eq 0 ]
  then
    echo "No model name supplied"
    echo "Usage: ./generate-coreml-model.sh <model-name>"
    exit 1
fi

mname="$1"

wd=$(dirname "$0")
cd "$wd/../"

python3 models/convert-whisper-to-coreml.py --model $mname --encoder-only True

xcrun coremlc compile models/coreml-encoder-${mname}.mlpackage models/
rm -rf models/ggml-${mname}-encoder.mlmodelc
mv -v models/coreml-encoder-${mname}.mlmodelc models/ggml-${mname}-encoder.mlmodelc

# TODO: decoder (sometime in the future maybe)
#xcrun coremlc compile models/whisper-decoder-${mname}.mlpackage models/
#rm -rf models/ggml-${mname}-decoder.mlmodelc
#mv -v models/coreml_decoder_${mname}.mlmodelc models/ggml-${mname}-decoder.mlmodelc
