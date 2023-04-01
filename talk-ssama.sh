./talk-llama \
    -mw ./models/ggml-small.en.bin \
    -ml ../llama.cpp/models/13B/ggml-model-q4_0.bin \
    --name-ni "Georgi" \
    --name-ai "SSaMA" \
    -t 8 -vid 2 --speak ./examples/talk-llama/speak.sh
