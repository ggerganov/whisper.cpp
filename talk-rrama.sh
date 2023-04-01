./talk-llama \
    -mw ./models/ggml-small.en.bin \
    -ml ../llama.cpp/models/13B/ggml-model-q4_0.bin \
    --name-ni "Georgi" \
    --name-ai "RRaMA" \
    -t 8 -vid 3 --speak ./examples/talk-llama/speak.sh
