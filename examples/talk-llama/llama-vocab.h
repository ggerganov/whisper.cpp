#pragma once

#include "llama-impl.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <set>

struct llm_tokenizer;

struct llama_vocab {
    using id    = llama_token;
    using token = std::string;
    using tattr = llama_token_attr;

    struct token_data {
        token text;
        float score;
        tattr attr;
    };

    uint32_t n_vocab = 0; // TODO: not great because has to keep in sync with hparams.n_vocab

    enum llama_vocab_type     type     = LLAMA_VOCAB_TYPE_SPM;
    enum llama_vocab_pre_type type_pre = LLAMA_VOCAB_PRE_TYPE_DEFAULT;

    int max_token_len = 0; // used for optimizing longest token search

    std::unordered_map<token, id> token_to_id;
    std::vector<token_data>       id_to_token;

    std::vector<id>    cache_special_tokens;
    std::vector<token> cache_token_to_piece; // llama_token_to_piece(special = true);

    std::map<std::pair<std::string, std::string>, int> bpe_ranks;

    // default LLaMA special tokens
    id special_bos_id  = 1;
    id special_eos_id  = 2;
    id special_unk_id  = 0;
    id special_sep_id  = -1;
    id special_pad_id  = -1;
    id special_cls_id  = -1;
    id special_mask_id = -1;

    id linefeed_id       = 13;
    id special_prefix_id = -1;
    id special_suffix_id = -1;
    id special_middle_id = -1;
    id special_eot_id    = -1; // TODO: move above after "eos_id", and here add "file separator" token
    id special_eom_id    = -1;

    // set of all tokens that cause "end of generation"
    std::set<id> special_eog_ids;

    // tokenizer flags
    bool tokenizer_add_space_prefix           = false;
    bool tokenizer_add_bos                    = false;
    bool tokenizer_add_eos                    = false;
    bool tokenizer_ignore_merges              = false;
    bool tokenizer_clean_spaces               = false;  // clean_up_tokenization_spaces
    bool tokenizer_remove_extra_whitespaces   = false;
    bool tokenizer_escape_whitespaces         = true;
    bool tokenizer_treat_whitespace_as_suffix = false;

    std::vector<char> precompiled_charsmap;

    llm_tokenizer * tokenizer = nullptr;

    llama_vocab() = default;
    ~llama_vocab();

    int find_bpe_rank(const std::string & token_left, const std::string & token_right) const;

    void init_tokenizer();
};

//
// internal API
//

// TODO: rename to llama_tokenize_impl
// TODO: This should probably be in llama.h
std::vector<llama_vocab::id> llama_tokenize_internal(
        const llama_vocab & vocab,
        std::string raw_text,
        bool add_special,
        bool parse_special = false);

// TODO: move the API below as member functions of llama_vocab
llama_token llama_byte_to_token_impl(const llama_vocab & vocab, uint8_t ch);

const char * llama_token_get_text_impl(const struct llama_vocab & vocab, llama_token token);

float llama_token_get_score_impl(const struct llama_vocab & vocab, llama_token token);

llama_token_attr llama_token_get_attr_impl(const struct llama_vocab & vocab, llama_token token);

bool llama_token_is_eog_impl(const struct llama_vocab & vocab, llama_token token);

bool llama_token_is_control_impl(const struct llama_vocab & vocab, llama_token token);

llama_token llama_token_bos_impl(const struct llama_vocab & vocab);
llama_token llama_token_eos_impl(const struct llama_vocab & vocab);
llama_token llama_token_cls_impl(const struct llama_vocab & vocab);
llama_token llama_token_sep_impl(const struct llama_vocab & vocab);
llama_token llama_token_nl_impl (const struct llama_vocab & vocab);
llama_token llama_token_pad_impl(const struct llama_vocab & vocab);

bool llama_add_bos_token_impl(const struct llama_vocab & vocab);
bool llama_add_eos_token_impl(const struct llama_vocab & vocab);

llama_token llama_token_prefix_impl(const struct llama_vocab & vocab);
llama_token llama_token_middle_impl(const struct llama_vocab & vocab);
llama_token llama_token_suffix_impl(const struct llama_vocab & vocab);
llama_token llama_token_eot_impl   (const struct llama_vocab & vocab);
llama_token llama_token_eom_impl   (const struct llama_vocab & vocab);

int32_t llama_tokenize_impl(
        const struct llama_vocab & vocab,
                      const char * text,
                         int32_t   text_len,
                     llama_token * tokens,
                         int32_t   n_tokens_max,
                            bool   add_special,
                            bool   parse_special);

// does not write null-terminator to buf
int32_t llama_token_to_piece_impl(
        const struct llama_vocab & vocab,
                     llama_token   token,
                            char * buf,
                         int32_t   length,
                         int32_t   lstrip,
                            bool   special);

int32_t llama_detokenize_impl(
        const struct llama_vocab & vocab,
               const llama_token * tokens,
                         int32_t   n_tokens,
                            char * text,
                         int32_t   text_len_max,
                            bool   remove_special,
                            bool   unparse_special);
