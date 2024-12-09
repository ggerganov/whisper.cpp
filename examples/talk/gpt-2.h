#pragma once

// TODO: Change to C-style API and move to ./examples for easy reuse.

#include "common.h"

#include <vector>
#include <map>
#include <string>
#include <regex>


struct gpt2_layer {
    // normalization
    struct ggml_tensor * ln_1_g;
    struct ggml_tensor * ln_1_b;

    struct ggml_tensor * ln_2_g;
    struct ggml_tensor * ln_2_b;

    // attention
    struct ggml_tensor * c_attn_attn_w;
    struct ggml_tensor * c_attn_attn_b;

    struct ggml_tensor * c_attn_proj_w;
    struct ggml_tensor * c_attn_proj_b;

    // mlp
    struct ggml_tensor * c_mlp_fc_w;
    struct ggml_tensor * c_mlp_fc_b;

    struct ggml_tensor * c_mlp_proj_w;
    struct ggml_tensor * c_mlp_proj_b;
};

constexpr int N_THREAD = 8;

// default hparams (GPT-2 117M)
struct gpt2_hparams {
    int32_t n_vocab = 50257;
    int32_t n_ctx   = 1024;
    int32_t n_embd  = 768;
    int32_t n_head  = 12;
    int32_t n_layer = 12;
    int32_t ftype   = 1;
    float   eps     = 1e-5f;
};

struct gpt2_model {
    gpt2_hparams hparams;

    // normalization
    struct ggml_tensor * ln_f_g;
    struct ggml_tensor * ln_f_b;

    struct ggml_tensor * wte;     // position embedding
    struct ggml_tensor * wpe;     //    token embedding
    struct ggml_tensor * lm_head; // language model head

    std::vector<gpt2_layer> layers;

    // key + value memory
    struct ggml_tensor * memory_k;
    struct ggml_tensor * memory_v;

    //
    struct ggml_context * ctx_w;
    struct ggml_context * ctx_kv;

    ggml_backend* backend = NULL;

    ggml_backend_buffer * buffer_w;
    ggml_backend_buffer * buffer_kv;

    std::map<std::string, struct ggml_tensor *> tensors;
};


struct gpt2_context {
    std::string prompt_base = R"(Hello, how are you?
I'm fine, thanks. How are you?
Thanks, I'm fine too. What are you doing?
I'm just sitting here.
It's a lovely day, isn't it?
Yes, it is. I love the weather this time of year.
I wish it would rain a little bit.
Me too.
)";

    std::mt19937 rng;

    gpt_vocab vocab;
    gpt2_model model;

    int32_t n_threads = std::min(N_THREAD, (int) std::thread::hardware_concurrency());

    // sampling parameters
    int32_t top_k = 5;
    float   top_p = 0.9f;
    float   temp  = 1.0f;
};

bool gpt2_model_load(const std::string &fname, gpt2_model &model, gpt_vocab &vocab, int n_ctx, int n_gpu_layers);
struct gpt2_context *gpt2_init(const char *path_model);
void gpt2_free(struct gpt2_context * ctx);

const char * gpt2_get_prompt(struct gpt2_context * ctx);
void gpt2_set_prompt(struct gpt2_context * ctx, const char * prompt);

std::vector<gpt_vocab::id> gpt2_tokenize(const gpt2_context * ctx, const char * text);


std::string gpt2_gen_text(gpt2_context * ctx, const char * text, int max_tokens, ggml_gallocr* allocr);
struct ggml_cgraph *gpt2_graph(
    const gpt2_model &model,
    const int n_past,
    const int n_tokens);

bool gpt2_eval(
    const gpt2_model &model,
    ggml_gallocr_t allocr,
    const int n_threads,
    const int n_past,
    const std::vector<gpt_vocab::id> &embd_inp,
    std::vector<float> &embd_w);