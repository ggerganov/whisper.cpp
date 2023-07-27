#pragma once

// TODO: Change to C-style API and move to ./examples for easy reuse.

#include "common.h"

#include <vector>
#include <map>
#include <string>

struct gpt2_context;

struct gpt2_context * gpt2_init(const char * path_model);
void gpt2_free(struct gpt2_context * ctx);

const char * gpt2_get_prompt(struct gpt2_context * ctx);
void gpt2_set_prompt(struct gpt2_context * ctx, const char * prompt);

std::vector<gpt_vocab::id> gpt2_tokenize(const gpt2_context * ctx, const char * text);

std::string gpt2_gen_text(gpt2_context * ctx, const char * text, int max_tokens);
