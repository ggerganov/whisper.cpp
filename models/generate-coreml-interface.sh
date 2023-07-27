#!/bin/bash
#
# This generates:
#   - coreml/whisper-encoder-impl.h and coreml/whisper-encoder-impl.m
#   - coreml/whisper-decoder-impl.h and coreml/whisper-decoder-impl.m
#

wd=$(dirname "$0")
cd "$wd/../"

python3 models/convert-whisper-to-coreml.py --model tiny.en

mv -v models/coreml-encoder-tiny.en.mlpackage models/whisper-encoder-impl.mlpackage
xcrun coremlc generate models/whisper-encoder-impl.mlpackage coreml/
mv coreml/whisper_encoder_impl.h coreml/whisper-encoder-impl.h
mv coreml/whisper_encoder_impl.m coreml/whisper-encoder-impl.m
sed -i '' 's/whisper_encoder_impl\.h/whisper-encoder-impl.h/g' coreml/whisper-encoder-impl.m
sed -i '' 's/whisper_encoder_impl\.m/whisper-encoder-impl.m/g' coreml/whisper-encoder-impl.m
sed -i '' 's/whisper_encoder_impl\.h/whisper-encoder-impl.h/g' coreml/whisper-encoder-impl.h

mv -v models/coreml-decoder-tiny.en.mlpackage models/whisper-decoder-impl.mlpackage
xcrun coremlc generate models/whisper-decoder-impl.mlpackage coreml/
mv coreml/whisper_decoder_impl.h coreml/whisper-decoder-impl.h
mv coreml/whisper_decoder_impl.m coreml/whisper-decoder-impl.m
sed -i '' 's/whisper_decoder_impl\.h/whisper-decoder-impl.h/g' coreml/whisper-decoder-impl.m
sed -i '' 's/whisper_decoder_impl\.m/whisper-decoder-impl.m/g' coreml/whisper-decoder-impl.m
sed -i '' 's/whisper_decoder_impl\.h/whisper-decoder-impl.h/g' coreml/whisper-decoder-impl.h

rm -rfv models/whisper-encoder-impl.mlpackage models/whisper-decoder-impl.mlpackage
