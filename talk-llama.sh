./talk-llama \
    -mw ./models/ggml-small.en.bin \
    -ml ../llama.cpp/models/13B/ggml-model-q4_0.bin \
    --name-ni "Georgi" \
    --name-ai "LLaMA" \
    -t 8 -vid 0 --speak ./examples/talk-llama/speak.sh
