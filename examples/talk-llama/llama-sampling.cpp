#include "llama-sampling.h"

#include "llama-vocab.h"
#include "llama-grammar.h"

#include <algorithm>
#include <cassert>
#include <cfloat>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <numeric>
#include <random>
#include <unordered_map>

static int llama_sample_dist(llama_token_data_array * cur_p, std::mt19937 & rng) {
    // iterator for the probabilities
#ifdef __GNUC__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#endif

    struct probs_iterator {
        typedef std::input_iterator_tag iterator_category;
        typedef float value_type;
        typedef float * pointer;
        typedef float & reference;
        typedef ptrdiff_t difference_type;

        const llama_token_data * data;

        bool operator==(const probs_iterator & other) const { return data == other.data; }
        bool operator!=(const probs_iterator & other) const { return data != other.data; }
        const float & operator*() const { return data->p; }
        probs_iterator & operator++() { ++data; return *this; }
        probs_iterator operator++(int) { probs_iterator tmp = *this; ++data; return tmp; }
    };

#ifdef __GNUC__
    #pragma GCC diagnostic pop
#endif

    std::discrete_distribution<int> dist(probs_iterator{cur_p->data}, probs_iterator{cur_p->data + cur_p->size});

    return dist(rng);
}

/*
static void llama_log_softmax(float * array, size_t size) {
    float max_l = *std::max_element(array, array + size);
    float sum = 0.f;
    for (size_t i = 0; i < size; ++i) {
        float p = expf(array[i] - max_l);
        sum += p;
        array[i] = p;
    }

    for (size_t i = 0; i < size; ++i) {
        array[i] = logf(array[i] / sum);
    }
}
*/

static void llama_sampler_softmax_impl(llama_token_data_array * cur_p) {
    GGML_ASSERT(cur_p->size > 0);

    // Sort the logits in descending order
    if (!cur_p->sorted) {
        std::sort(cur_p->data, cur_p->data + cur_p->size, [](const llama_token_data & a, const llama_token_data & b) {
            return a.logit > b.logit;
        });
        cur_p->sorted = true;
    }

    float max_l = cur_p->data[0].logit;
    float cum_sum = 0.0f;

    for (size_t i = 0; i < cur_p->size; ++i) {
        float p = expf(cur_p->data[i].logit - max_l);
        cur_p->data[i].p = p;
        cum_sum += p;
    }

    for (size_t i = 0; i < cur_p->size; ++i) {
        cur_p->data[i].p /= cum_sum;
    }
}

static void llama_sampler_top_k_impl(llama_token_data_array * cur_p, int32_t k) {
    // TODO: move bucket sort to separate function so that top_p/tail_free/typical/softmax first is equally fast
    // if (k >= (int32_t)cur_p->size) {
    //     return;
    // }

    if (k <= 0) {
        k = cur_p->size;
    }

    k = std::min(k, (int) cur_p->size);

    // Sort scores in descending order
    if (!cur_p->sorted) {
        auto comp = [](const llama_token_data & a, const llama_token_data & b) {
            return a.logit > b.logit;
        };
        if (k <= 128) {
            std::partial_sort(cur_p->data, cur_p->data + k, cur_p->data + cur_p->size, comp);
        } else {
            constexpr int   nbuckets     = 128;
            constexpr float bucket_low   = -10.0f;
            constexpr float bucket_high  =  10.0f;
            constexpr float bucket_scale = nbuckets/(bucket_high - bucket_low);
            constexpr float bucket_inter = -bucket_low * bucket_scale;

            std::vector<int> bucket_idx(cur_p->size);
            std::vector<int> histo(nbuckets, 0);

            for (int i = 0; i < (int)cur_p->size; ++i) {
                const float val = cur_p->data[i].logit;
                int ib = int(bucket_scale * val + bucket_inter); //nbuckets * (val - bucket_low) / (bucket_high - bucket_low);
                ib = std::max(0, std::min(nbuckets-1, ib));
                bucket_idx[i] = ib;
                ++histo[ib];
            }
            int nhave = 0;
            int ib = nbuckets - 1;
            for ( ; ib >= 0; --ib) {
                nhave += histo[ib];
                if (nhave >= k) {
                    break;
                }
            }
            std::vector<llama_token_data> tmp_tokens(nhave);
            auto * ptr = tmp_tokens.data();
            std::vector<llama_token_data*> bucket_ptrs;
            bucket_ptrs.reserve(nbuckets - ib);
            for (int j = nbuckets - 1; j >= ib; --j) {
                bucket_ptrs.push_back(ptr);
                ptr += histo[j];
            }
            for (int i = 0; i < (int)cur_p->size; ++i) {
                int j = bucket_idx[i];
                if (j >= ib) {
                    *bucket_ptrs[nbuckets-1-j]++ = cur_p->data[i];
                }
            }

            ptr = tmp_tokens.data();
            int ndone = 0;
            for (int j = nbuckets-1; j > ib; --j) {
                std::sort(ptr, ptr + histo[j], comp);
                ptr += histo[j];
                ndone += histo[j];
            }
            std::partial_sort(ptr, ptr + k - ndone, ptr + histo[ib], comp);

            std::memcpy(cur_p->data, tmp_tokens.data(), k*sizeof(llama_token_data));

        }
        cur_p->sorted = true;
    }
    cur_p->size = k;
}

static uint32_t get_rng_seed(uint32_t seed) {
    if (seed == LLAMA_DEFAULT_SEED) {
        // use system clock if std::random_device is not a true RNG
        static bool is_rd_prng = std::random_device().entropy() == 0;
        if (is_rd_prng) {
            return (uint32_t) std::chrono::system_clock::now().time_since_epoch().count();
        }
        std::random_device rd;
        return rd();
    }
    return seed;
}

// llama_sampler API

const char * llama_sampler_name(const struct llama_sampler * smpl) {
    if (!smpl->iface) {
        return "(null)";
    }

    return smpl->iface->name(smpl);
}

void llama_sampler_accept(struct llama_sampler * smpl, llama_token token) {
    if (smpl->iface->accept) {
        smpl->iface->accept(smpl, token);
    }
}

void llama_sampler_apply(struct llama_sampler * smpl, struct llama_token_data_array * cur_p) {
    GGML_ASSERT(smpl->iface->apply);
    smpl->iface->apply(smpl, cur_p);
}

void llama_sampler_reset(struct llama_sampler * smpl) {
    if (smpl->iface->reset) {
        smpl->iface->reset(smpl);
    }
}

struct llama_sampler * llama_sampler_clone(const struct llama_sampler * smpl) {
    if (smpl->iface->clone) {
        return smpl->iface->clone(smpl);
    }

    if (smpl->ctx == nullptr) {
        return new llama_sampler {
            /* .iface = */ smpl->iface,
            /* .ctx   = */ nullptr,
        };
    }

    GGML_ABORT("the sampler does not support cloning");
}

void llama_sampler_free(struct llama_sampler * smpl) {
    if (smpl == nullptr) {
        return;
    }

    if (smpl->iface->free) {
        smpl->iface->free(smpl);
    }

    delete smpl;
}

llama_token llama_sampler_sample(struct llama_sampler * smpl, struct llama_context * ctx, int32_t idx) {
    const auto * logits = llama_get_logits_ith(ctx, idx);

    const int n_vocab = llama_n_vocab(llama_get_model(ctx));

    // TODO: do not allocate each time
    std::vector<llama_token_data> cur;
    cur.reserve(n_vocab);
    for (llama_token token_id = 0; token_id < n_vocab; token_id++) {
        cur.emplace_back(llama_token_data{token_id, logits[token_id], 0.0f});
    }

    llama_token_data_array cur_p = {
        /* .data       = */ cur.data(),
        /* .size       = */ cur.size(),
        /* .selected   = */ -1,
        /* .sorted     = */ false,
    };

    llama_sampler_apply(smpl, &cur_p);

    GGML_ASSERT(cur_p.selected >= 0 && cur_p.selected < (int32_t) cur_p.size);

    auto token = cur_p.data[cur_p.selected].id;

    llama_sampler_accept(smpl, token);

    return token;
}

// sampler chain

static const char * llama_sampler_chain_name(const struct llama_sampler * /*smpl*/) {
    return "chain";
}

static void llama_sampler_chain_accept(struct llama_sampler * smpl, llama_token token) {
    auto * chain = (llama_sampler_chain *) smpl->ctx;

    time_meas tm(chain->t_sample_us, chain->params.no_perf);

    for (auto * smpl : chain->samplers) {
        llama_sampler_accept(smpl, token);
    }

    chain->n_sample++;
}

static void llama_sampler_chain_apply(struct llama_sampler * smpl, llama_token_data_array * cur_p) {
    auto * chain = (llama_sampler_chain *) smpl->ctx;

    time_meas tm(chain->t_sample_us, chain->params.no_perf);

    for (auto * smpl : chain->samplers) {
        llama_sampler_apply(smpl, cur_p);
    }
}

static void llama_sampler_chain_reset(struct llama_sampler * smpl) {
    auto * chain = (llama_sampler_chain *) smpl->ctx;

    for (auto * smpl : chain->samplers) {
        llama_sampler_reset(smpl);
    }

    chain->t_sample_us = 0;
    chain->n_sample    = 0;
}

static struct llama_sampler * llama_sampler_chain_clone(const struct llama_sampler * smpl) {
    const auto * chain_src = (const llama_sampler_chain *) smpl->ctx;

    auto * result = llama_sampler_chain_init(chain_src->params);

    for (auto * smpl : chain_src->samplers) {
        llama_sampler_chain_add(result, llama_sampler_clone(smpl));
    }

    return result;
}

static void llama_sampler_chain_free(struct llama_sampler * smpl) {
    auto * chain = (llama_sampler_chain *) smpl->ctx;

    for (auto * smpl : chain->samplers) {
        llama_sampler_free(smpl);
    }

    delete chain;
}

static struct llama_sampler_i llama_sampler_chain_i = {
    /* .name   = */ llama_sampler_chain_name,
    /* .accept = */ llama_sampler_chain_accept,
    /* .apply  = */ llama_sampler_chain_apply,
    /* .reset  = */ llama_sampler_chain_reset,
    /* .clone  = */ llama_sampler_chain_clone,
    /* .free   = */ llama_sampler_chain_free,
};

struct llama_sampler * llama_sampler_chain_init(struct llama_sampler_chain_params params) {
    return new llama_sampler {
        /* .iface = */ &llama_sampler_chain_i,
        /* .ctx   = */ new llama_sampler_chain {
            /* .params      = */ params,
            /* .samplers    = */ {},
            /* .t_sample_us = */ 0,
            /* .n_sample    = */ 0,
        },
    };
}

void llama_sampler_chain_add(struct llama_sampler * chain, struct llama_sampler * smpl) {
    auto * p = (llama_sampler_chain *) chain->ctx;
    p->samplers.push_back(smpl);
}

struct llama_sampler * llama_sampler_chain_get(const struct llama_sampler * chain, int32_t i) {
    const auto * p = (const llama_sampler_chain *) chain->ctx;

    if (i < 0 || (size_t) i >= p->samplers.size()) {
        return nullptr;
    }

    return p->samplers[i];
}

struct llama_sampler * llama_sampler_chain_remove(struct llama_sampler * chain, int32_t i) {
    auto * p = (llama_sampler_chain *) chain->ctx;

    if (i < 0 || (size_t) i >= p->samplers.size()) {
        return nullptr;
    }

    auto * result = p->samplers[i];
    p->samplers.erase(p->samplers.begin() + i);

    return result;
}

int llama_sampler_chain_n(const struct llama_sampler * chain) {
    const auto * p = (const llama_sampler_chain *) chain->ctx;

    return p->samplers.size();
}

//
// samplers
//

// greedy

static const char * llama_sampler_greedy_name(const struct llama_sampler * /*smpl*/) {
    return "greedy";
}

static void llama_sampler_greedy_apply(struct llama_sampler * /*smpl*/, llama_token_data_array * cur_p) {
    cur_p->selected = 0;
    for (size_t i = 1; i < cur_p->size; ++i) {
        if (cur_p->data[i].logit > cur_p->data[cur_p->selected].logit) {
            cur_p->selected = i;
        }
    }
}

static struct llama_sampler_i llama_sampler_greedy_i = {
    /* .name   = */ llama_sampler_greedy_name,
    /* .accept = */ nullptr,
    /* .apply  = */ llama_sampler_greedy_apply,
    /* .reset  = */ nullptr,
    /* .clone  = */ nullptr,
    /* .free   = */ nullptr,
};

struct llama_sampler * llama_sampler_init_greedy() {
    return new llama_sampler {
        /* .iface = */ &llama_sampler_greedy_i,
        /* .ctx   = */ nullptr,
    };
}

// dist

struct llama_sampler_dist {
    const uint32_t seed;
          uint32_t seed_cur;

    std::mt19937 rng;
};

static const char * llama_sampler_dist_name(const struct llama_sampler * /*smpl*/) {
    return "dist";
}

static void llama_sampler_dist_apply(struct llama_sampler * smpl, llama_token_data_array * cur_p) {
    auto * ctx = (llama_sampler_dist *) smpl->ctx;
    cur_p->selected = llama_sample_dist(cur_p, ctx->rng);
}

static struct llama_sampler * llama_sampler_dist_clone(const struct llama_sampler * smpl) {
    const auto * ctx = (const llama_sampler_dist *) smpl->ctx;
    auto * result = llama_sampler_init_dist(ctx->seed);

    // copy the state
    {
        auto * result_ctx = (llama_sampler_dist *) result->ctx;

        result_ctx->rng = ctx->rng;
    }

    return result;
}

static void llama_sampler_dist_reset(struct llama_sampler * smpl) {
    auto * ctx = (llama_sampler_dist *) smpl->ctx;
    ctx->seed_cur = get_rng_seed(ctx->seed);
    ctx->rng.seed(ctx->seed_cur);
}

static void llama_sampler_dist_free(struct llama_sampler * smpl) {
    delete (llama_sampler_dist *) smpl->ctx;
}

static struct llama_sampler_i llama_sampler_dist_i = {
    /* .name   = */ llama_sampler_dist_name,
    /* .accept = */ nullptr,
    /* .apply  = */ llama_sampler_dist_apply,
    /* .reset  = */ llama_sampler_dist_reset,
    /* .clone  = */ llama_sampler_dist_clone,
    /* .free   = */ llama_sampler_dist_free,
};

struct llama_sampler * llama_sampler_init_dist(uint32_t seed) {
    auto seed_cur = get_rng_seed(seed);
    return new llama_sampler {
        /* .iface = */ &llama_sampler_dist_i,
        /* .ctx   = */ new llama_sampler_dist {
            /* .seed     = */ seed,
            /* .seed_cur = */ seed_cur,
            /* .rng      = */ std::mt19937(seed_cur),
        },
    };
}

// softmax

static const char * llama_sampler_softmax_name(const struct llama_sampler * /*smpl*/) {
    return "softmax";
}

static void llama_sampler_softmax_apply(struct llama_sampler * /*smpl*/, llama_token_data_array * cur_p) {
    llama_sampler_softmax_impl(cur_p);
}

static struct llama_sampler_i llama_sampler_softmax_i = {
    /* .name   = */ llama_sampler_softmax_name,
    /* .accept = */ nullptr,
    /* .apply  = */ llama_sampler_softmax_apply,
    /* .reset  = */ nullptr,
    /* .clone  = */ nullptr,
    /* .free   = */ nullptr,
};

struct llama_sampler * llama_sampler_init_softmax() {
    return new llama_sampler {
        /* .iface = */ &llama_sampler_softmax_i,
        /* .ctx   = */ nullptr,
    };
}

// top-k

struct llama_sampler_top_k {
    const int32_t k;
};

static const char * llama_sampler_top_k_name(const struct llama_sampler * /*smpl*/) {
    return "top-k";
}

static void llama_sampler_top_k_apply(struct llama_sampler * smpl, llama_token_data_array * cur_p) {
    const auto * ctx = (llama_sampler_top_k *) smpl->ctx;
    llama_sampler_top_k_impl(cur_p, ctx->k);
}

static struct llama_sampler * llama_sampler_top_k_clone(const struct llama_sampler * smpl) {
    const auto * ctx = (const llama_sampler_top_k *) smpl->ctx;
    return llama_sampler_init_top_k(ctx->k);
}

static void llama_sampler_top_k_free(struct llama_sampler * smpl) {
    delete (llama_sampler_top_k *) smpl->ctx;
}

static struct llama_sampler_i llama_sampler_top_k_i = {
    /* .name   = */ llama_sampler_top_k_name,
    /* .accept = */ nullptr,
    /* .apply  = */ llama_sampler_top_k_apply,
    /* .reset  = */ nullptr,
    /* .clone  = */ llama_sampler_top_k_clone,
    /* .free   = */ llama_sampler_top_k_free,
};

struct llama_sampler * llama_sampler_init_top_k(int32_t k) {
    return new llama_sampler {
        /* .iface = */ &llama_sampler_top_k_i,
        /* .ctx   = */ new llama_sampler_top_k {
            /* .k = */ k,
        },
    };
}

// top-p

struct llama_sampler_top_p {
    const float  p;
    const size_t min_keep;
};

static const char * llama_sampler_top_p_name(const struct llama_sampler * /*smpl*/) {
    return "top-p";
}

static void llama_sampler_top_p_apply(struct llama_sampler * smpl, llama_token_data_array * cur_p) {
    const auto * ctx = (llama_sampler_top_p *) smpl->ctx;

    if (ctx->p >= 1.0f) {
        return;
    }

    llama_sampler_softmax_impl(cur_p);

    // Compute the cumulative probabilities
    float cum_sum = 0.0f;
    size_t last_idx = cur_p->size;

    for (size_t i = 0; i < cur_p->size; ++i) {
        cum_sum += cur_p->data[i].p;

        // Check if the running sum is at least p or if we have kept at least min_keep tokens
        // we set the last index to i+1 to indicate that the current iterate should be included in the set
        if (cum_sum >= ctx->p && i + 1 >= ctx->min_keep) {
            last_idx = i + 1;
            break;
        }
    }

    // Resize the output vector to keep only the top-p tokens
    cur_p->size = last_idx;
}

static struct llama_sampler * llama_sampler_top_p_clone(const struct llama_sampler * smpl) {
    const auto * ctx = (const llama_sampler_top_p *) smpl->ctx;
    return llama_sampler_init_top_p(ctx->p, ctx->min_keep);
}

static void llama_sampler_top_p_free(struct llama_sampler * smpl) {
    delete (llama_sampler_top_p *) smpl->ctx;
}

static struct llama_sampler_i llama_sampler_top_p_i = {
    /* .name   = */ llama_sampler_top_p_name,
    /* .accept = */ nullptr,
    /* .apply  = */ llama_sampler_top_p_apply,
    /* .reset  = */ nullptr,
    /* .clone  = */ llama_sampler_top_p_clone,
    /* .free   = */ llama_sampler_top_p_free,
};

struct llama_sampler * llama_sampler_init_top_p(float p, size_t min_keep) {
    return new llama_sampler {
        /* .iface = */ &llama_sampler_top_p_i,
        /* .ctx   = */ new llama_sampler_top_p {
            /* .p        = */ p,
            /* .min_keep = */ min_keep,
        },
    };
}

// min-p

struct llama_sampler_min_p {
    const float  p;
    const size_t min_keep;
};

static const char * llama_sampler_min_p_name(const struct llama_sampler * /*smpl*/) {
    return "min-p";
}

static void llama_sampler_min_p_apply(struct llama_sampler * smpl, llama_token_data_array * cur_p) {
    const auto * ctx = (llama_sampler_min_p *) smpl->ctx;

    if (ctx->p <= 0.0f || !cur_p->size) {
        return;
    }

    bool min_p_applied = false;

    // if the cur_p aren't sorted, try the unsorted implementation first
    if (!cur_p->sorted) {
        std::vector<llama_token_data> filtered_tokens;

        float max_logit = -FLT_MAX;
        for (size_t i = 0; i < cur_p->size; ++i) {
            max_logit = std::max(max_logit, cur_p->data[i].logit);
        }
        const float min_logit = max_logit + logf(ctx->p); // min logit for p_i >= p * p_max

        for (size_t i = 0; i < cur_p->size; ++i) {
            if (cur_p->data[i].logit >= min_logit) {
                filtered_tokens.push_back(cur_p->data[i]);
            }
        }

        // if we have enough values the operation was a success
        if (filtered_tokens.size() >= ctx->min_keep) {
            memcpy(cur_p->data, filtered_tokens.data(), filtered_tokens.size()*sizeof(llama_token_data));
            cur_p->size = filtered_tokens.size();
            min_p_applied = true;
        }
    }

    // if the cur_p are sorted or the unsorted implementation failed, use this implementation
    if (!min_p_applied) {
        // Sort the logits in descending order
        if (!cur_p->sorted) {
            std::sort(cur_p->data, cur_p->data + cur_p->size, [](const llama_token_data & a, const llama_token_data & b) {
                return a.logit > b.logit;
            });
            cur_p->sorted = true;
        }

        const float min_logit = cur_p->data[0].logit + logf(ctx->p); // min logit for p_i >= p * p_max
        size_t i = 1; // first token always matches

        for (; i < cur_p->size; ++i) {
            if (cur_p->data[i].logit < min_logit && i >= ctx->min_keep) {
                break; // prob too small
            }
        }

        // Resize the output vector to keep only the matching tokens
        cur_p->size = i;
    }
}

static struct llama_sampler * llama_sampler_min_p_clone(const struct llama_sampler * smpl) {
    const auto * ctx = (const llama_sampler_min_p *) smpl->ctx;
    return llama_sampler_init_min_p(ctx->p, ctx->min_keep);
}

static void llama_sampler_min_p_free(struct llama_sampler * smpl) {
    delete (llama_sampler_min_p *) smpl->ctx;
}

static struct llama_sampler_i llama_sampler_min_p_i = {
    /* .name   = */ llama_sampler_min_p_name,
    /* .accept = */ nullptr,
    /* .apply  = */ llama_sampler_min_p_apply,
    /* .reset  = */ nullptr,
    /* .clone  = */ llama_sampler_min_p_clone,
    /* .free   = */ llama_sampler_min_p_free,
};

struct llama_sampler * llama_sampler_init_min_p(float p, size_t min_keep) {
    return new llama_sampler {
        /* .iface = */ &llama_sampler_min_p_i,
        /* .ctx   = */ new llama_sampler_min_p {
            /* .p        = */ p,
            /* .min_keep = */ min_keep,
        },
    };
}

// tail-free

struct llama_sampler_tail_free {
    const float  z;
    const size_t min_keep;
};

static const char * llama_sampler_tail_free_name(const struct llama_sampler * /*smpl*/) {
    return "tail-free";
}

static void llama_sampler_tail_free_apply(struct llama_sampler * smpl, llama_token_data_array * cur_p) {
    const auto * ctx = (llama_sampler_tail_free *) smpl->ctx;

    if (ctx->z >= 1.0f || cur_p->size <= 2) {
        return;
    }

    llama_sampler_softmax_impl(cur_p);

    // Compute the first and second derivatives
    std::vector<float> first_derivatives(cur_p->size - 1);
    std::vector<float> second_derivatives(cur_p->size - 2);

    for (size_t i = 0; i < first_derivatives.size(); ++i) {
        first_derivatives[i] = cur_p->data[i].p - cur_p->data[i + 1].p;
    }
    for (size_t i = 0; i < second_derivatives.size(); ++i) {
        second_derivatives[i] = first_derivatives[i] - first_derivatives[i + 1];
    }

    // Calculate absolute value of second derivatives
    for (size_t i = 0; i < second_derivatives.size(); ++i) {
        second_derivatives[i] = std::abs(second_derivatives[i]);
    }

    // Normalize the second derivatives
    {
        const float second_derivatives_sum = std::accumulate(second_derivatives.begin(), second_derivatives.end(), 0.0f);

        if (second_derivatives_sum > 1e-6f) {
            for (float & value : second_derivatives) {
                value /= second_derivatives_sum;
            }
        } else {
            for (float & value : second_derivatives) {
                value = 1.0f / second_derivatives.size();
            }
        }
    }

    float cum_sum = 0.0f;
    size_t last_idx = cur_p->size;
    for (size_t i = 0; i < second_derivatives.size(); ++i) {
        cum_sum += second_derivatives[i];

        // Check if the running sum is greater than z or if we have kept at least min_keep tokens
        if (cum_sum > ctx->z && i >= ctx->min_keep) {
            last_idx = i;
            break;
        }
    }

    // Resize the output vector to keep only the tokens above the tail location
    cur_p->size = last_idx;
}

static struct llama_sampler * llama_sampler_tail_free_clone(const struct llama_sampler * smpl) {
    const auto * ctx = (const llama_sampler_tail_free *) smpl->ctx;
    return llama_sampler_init_tail_free(ctx->z, ctx->min_keep);
}

static void llama_sampler_tail_free_free(struct llama_sampler * smpl) {
    delete (llama_sampler_tail_free *) smpl->ctx;
}

static struct llama_sampler_i llama_sampler_tail_free_i = {
    /* .name   = */ llama_sampler_tail_free_name,
    /* .accept = */ nullptr,
    /* .apply  = */ llama_sampler_tail_free_apply,
    /* .reset  = */ nullptr,
    /* .clone  = */ llama_sampler_tail_free_clone,
    /* .free   = */ llama_sampler_tail_free_free,
};

struct llama_sampler * llama_sampler_init_tail_free(float z, size_t min_keep) {
    return new llama_sampler {
        /* .iface = */ &llama_sampler_tail_free_i,
        /* .ctx   = */ new llama_sampler_tail_free {
            /* .z        = */ z,
            /*. min_keep = */ min_keep,
        },
    };
}

// typical

struct llama_sampler_typical {
    const float  p;
    const size_t min_keep;
};

static const char * llama_sampler_typical_name(const struct llama_sampler * /*smpl*/) {
    return "typical";
}

static void llama_sampler_typical_apply(struct llama_sampler * smpl, llama_token_data_array * cur_p) {
    const auto * ctx = (llama_sampler_typical *) smpl->ctx;

    // Reference implementation:
    // https://github.com/huggingface/transformers/compare/main...cimeister:typical-sampling:typical-pr
    if (ctx->p >= 1.0f) {
        return;
    }

    // Compute the softmax of logits and calculate entropy
    llama_sampler_softmax_impl(cur_p);

    float entropy = 0.0f;
    for (size_t i = 0; i < cur_p->size; ++i) {
        entropy += -cur_p->data[i].p * logf(cur_p->data[i].p);
    }

    // Compute the absolute difference between negative log probability and entropy for each candidate
    std::vector<float> shifted_scores;
    for (size_t i = 0; i < cur_p->size; ++i) {
        float shifted_score = fabsf(-logf(cur_p->data[i].p) - entropy);
        shifted_scores.push_back(shifted_score);
    }

    // Sort tokens based on the shifted_scores and their corresponding indices
    std::vector<size_t> indices(cur_p->size);
    std::iota(indices.begin(), indices.end(), 0);

    std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
        return shifted_scores[a] < shifted_scores[b];
    });

    // Compute the cumulative probabilities
    float cum_sum = 0.0f;
    size_t last_idx = indices.size();

    for (size_t i = 0; i < indices.size(); ++i) {
        size_t idx = indices[i];
        cum_sum += cur_p->data[idx].p;

        // Check if the running sum is greater than typical or if we have kept at least min_keep tokens
        if (cum_sum > ctx->p && i >= ctx->min_keep - 1) {
            last_idx = i + 1;
            break;
        }
    }

    // Resize the output vector to keep only the locally typical tokens
    std::vector<llama_token_data> cur_p_new;
    for (size_t i = 0; i < last_idx; ++i) {
        size_t idx = indices[i];
        cur_p_new.push_back(cur_p->data[idx]);
    }

    // Replace the data in cur_p with the cur_p_new data
    std::copy(cur_p_new.begin(), cur_p_new.end(), cur_p->data);
    cur_p->size = cur_p_new.size();
    cur_p->sorted = false;
}

static struct llama_sampler * llama_sampler_typical_clone(const struct llama_sampler * smpl) {
    const auto * ctx = (const llama_sampler_typical *) smpl->ctx;
    return llama_sampler_init_typical(ctx->p, ctx->min_keep);
}

static void llama_sampler_typical_free(struct llama_sampler * smpl) {
    delete (llama_sampler_typical *) smpl->ctx;
}

static struct llama_sampler_i llama_sampler_typical_i = {
    /* .name   = */ llama_sampler_typical_name,
    /* .accept = */ nullptr,
    /* .apply  = */ llama_sampler_typical_apply,
    /* .reset  = */ nullptr,
    /* .clone  = */ llama_sampler_typical_clone,
    /* .free   = */ llama_sampler_typical_free,
};

struct llama_sampler * llama_sampler_init_typical(float p, size_t min_keep) {
    return new llama_sampler {
        /* .iface = */ &llama_sampler_typical_i,
        /* .ctx   = */ new llama_sampler_typical {
            /* .p        = */ p,
            /* .min_keep = */ min_keep,
        },
    };
}

// temp

struct llama_sampler_temp {
    const float temp;
};

static const char * llama_sampler_temp_name(const struct llama_sampler * /*smpl*/) {
    return "temp";
}

static void llama_sampler_temp_apply(struct llama_sampler * smpl, llama_token_data_array * cur_p) {
    const auto * ctx = (llama_sampler_temp *) smpl->ctx;
    for (size_t i = 0; i < cur_p->size; ++i) {
        cur_p->data[i].logit /= ctx->temp;
    }
}

static struct llama_sampler * llama_sampler_temp_clone(const struct llama_sampler * smpl) {
    const auto * ctx = (const llama_sampler_temp *) smpl->ctx;
    return llama_sampler_init_temp(ctx->temp);
}

static void llama_sampler_temp_free(struct llama_sampler * smpl) {
    delete (llama_sampler_temp *) smpl->ctx;
}

static struct llama_sampler_i llama_sampler_temp_i = {
    /* .name   = */ llama_sampler_temp_name,
    /* .accept = */ nullptr,
    /* .apply  = */ llama_sampler_temp_apply,
    /* .reset  = */ nullptr,
    /* .clone  = */ llama_sampler_temp_clone,
    /* .free   = */ llama_sampler_temp_free,
};

struct llama_sampler * llama_sampler_init_temp(float temp) {
    return new llama_sampler {
        /* .iface = */ &llama_sampler_temp_i,
        /* .ctx   = */ new llama_sampler_temp {
            /*.temp = */ temp,
        },
    };
}

// temp-ext

struct llama_sampler_temp_ext {
    const float temp;
    const float delta;
    const float exponent;
};

static const char * llama_sampler_temp_ext_name(const struct llama_sampler * /*smpl*/) {
    return "temp-ext";
}

static void llama_sampler_temp_ext_apply(struct llama_sampler * smpl, llama_token_data_array * cur_p) {
    const auto * ctx = (llama_sampler_temp_ext *) smpl->ctx;
    if (ctx->delta > 0) {
        const float min_temp = std::max(0.0f, ctx->temp - ctx->delta);
        const float max_temp = ctx->temp + ctx->delta;
        float exponent_val = ctx->exponent;

        // no need to do anything if there is only one (or zero) candidates
        if (cur_p->size <= 1) {
            return;
        }

        // Calculate maximum possible entropy
        float max_entropy = -logf(1.0f / cur_p->size);

        llama_sampler_softmax_impl(cur_p);

        // Calculate entropy of the softmax probabilities
        float entropy = 0.0f;
        for (size_t i = 0; i < cur_p->size; ++i) {
            float prob = cur_p->data[i].p;
            if (prob > 0.0f) { // Ensure no log(0)
                entropy -= prob * logf(prob);
            }
        }

        // Normalize the entropy (max_entropy cannot be 0 here because we checked cur_p->size != 1 above)
        float normalized_entropy = entropy / max_entropy;

        // Map the normalized entropy to the desired temperature range using the power function
        float dyn_temp = min_temp + (max_temp - min_temp) * powf(normalized_entropy, exponent_val);

    #ifdef DEBUG
        LLAMA_LOG_INFO("Your text maxtemp value is: %f\n", max_temp);
        LLAMA_LOG_INFO("Entropy: %f\n", entropy);
        LLAMA_LOG_INFO("Max Possible Entropy: %f\n", max_entropy);
        LLAMA_LOG_INFO("Normalized Entropy: %f\n", normalized_entropy);
        LLAMA_LOG_INFO("Exponent: %f\n", exponent_val);
        LLAMA_LOG_INFO("Dynamic Temperature (dyn_temp): %f\n", dyn_temp);
    #endif

        // Apply the dynamically calculated temperature scaling
        for (size_t i = 0; i < cur_p->size; ++i) {
            cur_p->data[i].logit /= dyn_temp;
        }

        // Re-compute softmax probabilities after scaling logits with dynamic temperature
        const double max_l_double = cur_p->data[0].logit;

        double cum_sum_double = 0.0;
        for (size_t i = 0; i < cur_p->size; ++i) {
            double p = exp(cur_p->data[i].logit - max_l_double);
            cur_p->data[i].p = p; // Store the scaled probability
            cum_sum_double += p;
        }

        for (size_t i = 0; i < cur_p->size; ++i) {
            cur_p->data[i].p /= cum_sum_double; // Re-normalize the probabilities
        }

    #ifdef DEBUG
        // Print the updated top 25 probabilities after temperature scaling
        LLAMA_LOG_INFO("\nUpdated Top 25 Probabilities After Dynamic Temperature Scaling (in percentages):\n");
        for (size_t i = 0; i < 25 && i < cur_p->size; ++i) {
            LLAMA_LOG_INFO("Token %zu: %f%%\n", i + 1, cur_p->data[i].p * 100.0f);
        }
    #endif
    } else {
        for (size_t i = 0; i < cur_p->size; ++i) {
            cur_p->data[i].logit /= ctx->temp;
        }
    }
}

static struct llama_sampler * llama_sampler_temp_ext_clone(const struct llama_sampler * smpl) {
    const auto * ctx = (const llama_sampler_temp_ext *) smpl->ctx;
    return llama_sampler_init_temp_ext(ctx->temp, ctx->delta, ctx->exponent);
}

static void llama_sampler_temp_ext_free(struct llama_sampler * smpl) {
    delete (llama_sampler_temp_ext *) smpl->ctx;
}

static struct llama_sampler_i llama_sampler_temp_ext_i = {
    /* .name   = */ llama_sampler_temp_ext_name,
    /* .accept = */ nullptr,
    /* .apply  = */ llama_sampler_temp_ext_apply,
    /* .reset  = */ nullptr,
    /* .clone  = */ llama_sampler_temp_ext_clone,
    /* .free   = */ llama_sampler_temp_ext_free,
};

struct llama_sampler * llama_sampler_init_temp_ext(float temp, float delta, float exponent) {
    return new llama_sampler {
        /* .iface = */ &llama_sampler_temp_ext_i,
        /* .ctx   = */ new llama_sampler_temp_ext {
            /* .temp     = */ temp,
            /* .delta    = */ delta,
            /* .exponent = */ exponent,
        },
    };
}

// mirostat

struct llama_sampler_mirostat {
    const int32_t n_vocab;

    const uint32_t seed;
          uint32_t seed_cur;

    const float tau;
    const float eta;

    const int32_t m;

    float mu;

    std::mt19937 rng;
};

static const char * llama_sampler_mirostat_name(const struct llama_sampler * /*smpl*/) {
    return "mirostat";
}

static void llama_sampler_mirostat_apply(struct llama_sampler * smpl, llama_token_data_array * cur_p) {
    auto * ctx = (llama_sampler_mirostat *) smpl->ctx;

    llama_sampler_softmax_impl(cur_p);

    // Estimate s_hat using the most probable m tokens
    float s_hat = 0.0;
    float sum_ti_bi = 0.0;
    float sum_ti_sq = 0.0;
    for (size_t i = 0; i < size_t(ctx->m - 1) && i < cur_p->size - 1; ++i) {
        float t_i = logf(float(i + 2) / float(i + 1));
        float b_i = logf(cur_p->data[i].p / cur_p->data[i + 1].p);
        sum_ti_bi += t_i * b_i;
        sum_ti_sq += t_i * t_i;
    }
    s_hat = sum_ti_bi / sum_ti_sq;

    // Compute k from the estimated s_hat and target surprise value
    float epsilon_hat = s_hat - 1;
    float k = powf((epsilon_hat * powf(2, ctx->mu)) / (1 - powf(ctx->n_vocab, -epsilon_hat)), 1 / s_hat);

    llama_sampler_top_k_impl(cur_p, std::max(int(k), 1));
    llama_sampler_softmax_impl(cur_p);

    const int idx = llama_sample_dist(cur_p, ctx->rng);

    cur_p->selected = idx;

    float observed_surprise = -log2f(cur_p->data[idx].p);
    float e = observed_surprise - ctx->tau;

    // Update mu using the learning rate and error
    ctx->mu = ctx->mu - ctx->eta * e;
}

static struct llama_sampler * llama_sampler_mirostat_clone(const struct llama_sampler * smpl) {
    const auto * ctx = (const llama_sampler_mirostat *) smpl->ctx;
    auto * result = llama_sampler_init_mirostat(ctx->n_vocab, ctx->seed, ctx->tau, ctx->eta, ctx->m);

    // copy the state
    {
        auto * result_ctx = (llama_sampler_mirostat *) smpl->ctx;

        result_ctx->mu  = ctx->mu;
        result_ctx->rng = ctx->rng;
    }

    return result;
}

static void llama_sampler_mirostat_reset(struct llama_sampler * smpl) {
    auto * ctx = (llama_sampler_mirostat *) smpl->ctx;
    ctx->mu = 2.0f*ctx->tau;
    ctx->seed_cur = get_rng_seed(ctx->seed);
    ctx->rng.seed(ctx->seed_cur);
}

static void llama_sampler_mirostat_free(struct llama_sampler * smpl) {
    delete (llama_sampler_mirostat *) smpl->ctx;
}

static struct llama_sampler_i llama_sampler_mirostat_i = {
    /* .name   = */ llama_sampler_mirostat_name,
    /* .accept = */ nullptr,
    /* .apply  = */ llama_sampler_mirostat_apply,
    /* .reset  = */ llama_sampler_mirostat_reset,
    /* .clone  = */ llama_sampler_mirostat_clone,
    /* .free   = */ llama_sampler_mirostat_free,
};

struct llama_sampler * llama_sampler_init_mirostat(int32_t n_vocab, uint32_t seed, float tau, float eta, int32_t m) {
    auto seed_cur = get_rng_seed(seed);
    return new llama_sampler {
        /* .iface = */ &llama_sampler_mirostat_i,
        /* .ctx   = */ new llama_sampler_mirostat {
            /* .n_vocab  = */ n_vocab,
            /* .seed     = */ seed,
            /* .seed_cur = */ seed_cur,
            /* .tau      = */ tau,
            /* .eta      = */ eta,
            /* .m        = */ m,
            /* .mu       = */ 2.0f*tau,
            /* .rng      = */ std::mt19937(seed_cur),
        },
    };
}

// mirostat v2

struct llama_sampler_mirostat_v2 {
    const uint32_t seed;
          uint32_t seed_cur;

    const float tau;
    const float eta;

    float mu;

    std::mt19937 rng;
};

static const char * llama_sampler_mirostat_v2_name(const struct llama_sampler * /*smpl*/) {
    return "mirostat-v2";
}

static void llama_sampler_mirostat_v2_apply(struct llama_sampler * smpl, llama_token_data_array * cur_p) {
    auto * ctx = (llama_sampler_mirostat_v2 *) smpl->ctx;

    llama_sampler_softmax_impl(cur_p);

    // Truncate the words with surprise values greater than mu
    cur_p->size = std::distance(cur_p->data, std::find_if(cur_p->data, cur_p->data + cur_p->size, [&](const llama_token_data & candidate) {
        return -log2f(candidate.p) > ctx->mu;
    }));

    if (cur_p->size == 0) {
        cur_p->size = 1;
    }

    // Normalize the probabilities of the remaining words
    llama_sampler_softmax_impl(cur_p);

    const int idx = llama_sample_dist(cur_p, ctx->rng);

    cur_p->selected = idx;

    float observed_surprise = -log2f(cur_p->data[idx].p);
    float e = observed_surprise - ctx->tau;

    // Update mu using the learning rate and error
    ctx->mu = ctx->mu - ctx->eta * e;
}

static void llama_sampler_mirostat_v2_reset(struct llama_sampler * smpl) {
    auto * ctx = (llama_sampler_mirostat_v2 *) smpl->ctx;
    ctx->mu = 2.0f*ctx->tau;
    ctx->seed_cur = get_rng_seed(ctx->seed);
    ctx->rng.seed(ctx->seed_cur);
}

static struct llama_sampler * llama_sampler_mirostat_v2_clone(const struct llama_sampler * smpl) {
    const auto * ctx = (const llama_sampler_mirostat_v2 *) smpl->ctx;

    auto * result = llama_sampler_init_mirostat_v2(ctx->seed, ctx->tau, ctx->eta);

    // copy the state
    {
        auto * result_ctx = (llama_sampler_mirostat_v2 *) result->ctx;

        result_ctx->mu  = ctx->mu;
        result_ctx->rng = ctx->rng;
    }

    return result;
}

static void llama_sampler_mirostat_v2_free(struct llama_sampler * smpl) {
    delete (llama_sampler_mirostat_v2 *) smpl->ctx;
}

static struct llama_sampler_i llama_sampler_mirostat_v2_i = {
    /* .name   = */ llama_sampler_mirostat_v2_name,
    /* .accept = */ nullptr,
    /* .apply  = */ llama_sampler_mirostat_v2_apply,
    /* .reset  = */ llama_sampler_mirostat_v2_reset,
    /* .clone  = */ llama_sampler_mirostat_v2_clone,
    /* .free   = */ llama_sampler_mirostat_v2_free,
};

struct llama_sampler * llama_sampler_init_mirostat_v2(uint32_t seed, float tau, float eta) {
    auto seed_cur = get_rng_seed(seed);
    return new llama_sampler {
        /* .iface = */ &llama_sampler_mirostat_v2_i,
        /* .ctx   = */ new llama_sampler_mirostat_v2 {
            /* .seed     = */ seed,
            /* .seed_cur = */ seed_cur,
            /* .tau      = */ tau,
            /* .eta      = */ eta,
            /* .mu       = */ 2.0f*tau,
            /* .rng      = */ std::mt19937(seed_cur),
        },
    };
}

// grammar

struct llama_sampler_grammar {
    const struct llama_vocab * vocab;

    std::string grammar_str;
    std::string grammar_root;

    struct llama_grammar * grammar;
};

static const char * llama_sampler_grammar_name(const struct llama_sampler * /*smpl*/) {
    return "grammar";
}

static void llama_sampler_grammar_accept_impl(struct llama_sampler * smpl, llama_token token) {
    auto * ctx = (llama_sampler_grammar *) smpl->ctx;
    if (ctx->grammar) {
        llama_grammar_accept_impl(*ctx->grammar, token);
    }
}

static void llama_sampler_grammar_apply(struct llama_sampler * smpl, llama_token_data_array * cur_p) {
    auto * ctx = (llama_sampler_grammar *) smpl->ctx;
    if (ctx->grammar) {
        llama_grammar_apply_impl(*ctx->grammar, cur_p);
    }
}

static void llama_sampler_grammar_reset(struct llama_sampler * smpl) {
    auto * ctx = (llama_sampler_grammar *) smpl->ctx;
    if (!ctx->grammar) {
        return;
    }

    auto * grammar_new = llama_grammar_init_impl(ctx->grammar->vocab, ctx->grammar_str.c_str(), ctx->grammar_root.c_str());

    llama_grammar_free_impl(ctx->grammar);
    ctx->grammar = grammar_new;
}

static struct llama_sampler * llama_sampler_grammar_clone(const struct llama_sampler * smpl) {
    const auto * ctx = (const llama_sampler_grammar *) smpl->ctx;

    auto * result = llama_sampler_init_grammar_impl(*ctx->vocab, nullptr, nullptr);

    // copy the state
    {
        auto * result_ctx = (llama_sampler_grammar *) result->ctx;

        if (ctx->grammar) {
            result_ctx->grammar_str  = ctx->grammar_str;
            result_ctx->grammar_root = ctx->grammar_root;

            result_ctx->grammar = llama_grammar_clone_impl(*ctx->grammar);
        }
    }

    return result;
}

static void llama_sampler_grammar_free(struct llama_sampler * smpl) {
    const auto * ctx = (llama_sampler_grammar *) smpl->ctx;

    if (ctx->grammar) {
        llama_grammar_free_impl(ctx->grammar);
    }

    delete ctx;
}

static struct llama_sampler_i llama_sampler_grammar_i = {
    /* .name   = */ llama_sampler_grammar_name,
    /* .accept = */ llama_sampler_grammar_accept_impl,
    /* .apply  = */ llama_sampler_grammar_apply,
    /* .reset  = */ llama_sampler_grammar_reset,
    /* .clone  = */ llama_sampler_grammar_clone,
    /* .free   = */ llama_sampler_grammar_free,
};

struct llama_sampler * llama_sampler_init_grammar_impl(const struct llama_vocab & vocab, const char * grammar_str, const char * grammar_root) {
    auto * ctx = new llama_sampler_grammar;

    if (grammar_str != nullptr && grammar_str[0] != '\0') {
        *ctx = {
            /* .vocab        = */ &vocab,
            /* .grammar_str  = */ grammar_str,
            /* .grammar_root = */ grammar_root,
            /* .grammar      = */ llama_grammar_init_impl(&vocab, grammar_str, grammar_root),
        };
    } else {
        *ctx = {
            /* .vocab        = */ &vocab,
            /* .grammar_str  = */ {},
            /* .grammar_root = */ {},
            /* .grammar      = */ nullptr,
        };
    }

    return new llama_sampler {
        /* .iface = */ &llama_sampler_grammar_i,
        /* .ctx   = */ ctx,
    };
}

// penalties

struct llama_sampler_penalties {
    const int32_t     n_vocab;
    const llama_token special_eos_id;
    const llama_token linefeed_id;

    const int32_t penalty_last_n;
    const float   penalty_repeat;
    const float   penalty_freq;
    const float   penalty_present;

    const bool    penalize_nl;
    const bool    ignore_eos;

    ring_buffer<llama_token> prev;
};

static const char * llama_sampler_penalties_name(const struct llama_sampler * /*smpl*/) {
    return "penalties";
}

static void llama_sampler_penalties_accept(struct llama_sampler * smpl, llama_token token) {
    auto * ctx = (llama_sampler_penalties *) smpl->ctx;
    if (ctx->penalty_last_n == 0) {
        return;
    }

    ctx->prev.push_back(token);
}

static void llama_sampler_penalties_apply(struct llama_sampler * smpl, llama_token_data_array * cur_p) {
    auto * ctx = (llama_sampler_penalties *) smpl->ctx;

    if (ctx->ignore_eos) {
        assert(ctx->special_eos_id >= 0);

        // optimistically check if the candidates are not yet sorted/shuffled/truncated
        if (cur_p->size > (size_t) ctx->special_eos_id && cur_p->data[ctx->special_eos_id].id == ctx->special_eos_id) {
            cur_p->data[ctx->special_eos_id].logit = -INFINITY;
        } else {
            // else, search for the special EOS token
            for (size_t i = 0; i < cur_p->size; ++i) {
                if (cur_p->data[i].id == ctx->special_eos_id) {
                    cur_p->data[i].logit = -INFINITY;
                    break;
                }
            }
        }
    }

    if ((ctx->penalty_last_n == 0) ||
        (ctx->penalty_repeat == 1.0f && ctx->penalty_freq == 0.0f && ctx->penalty_present == 0.0f)) {
        return;
    }

    bool nl_found = false;
    size_t nl_idx = 0;
    float nl_logit = -INFINITY;
    if (!ctx->penalize_nl) {
        assert(ctx->linefeed_id >= 0);

        // optimistically check if the candidates are not yet sorted/shuffled/truncated
        if (cur_p->size > (size_t) ctx->linefeed_id && cur_p->data[ctx->linefeed_id].id == ctx->linefeed_id) {
            nl_found = true;
            nl_idx = ctx->linefeed_id;
            nl_logit = cur_p->data[ctx->linefeed_id].logit;
        } else {
            // else, search for the linefeed token
            for (size_t i = 0; i < cur_p->size; ++i) {
                if (cur_p->data[i].id == ctx->linefeed_id) {
                    nl_found = true;
                    nl_idx = i;
                    nl_logit = cur_p->data[i].logit;
                    break;
                }
            }
        }
    }

    // Create a frequency map to count occurrences of each token in last_tokens
    // TODO: optimize this by maintaining the token count in the sampler context
    using llama_token_cnt = std::unordered_map<llama_token, int>;
    llama_token_cnt token_count;

    for (int i = 0; i < std::min<int>(ctx->penalty_last_n, ctx->prev.size()); ++i) {
        token_count[ctx->prev.rat(i)]++;
    }

    // Apply frequency and presence penalties to the cur_p
    for (size_t i = 0; i < cur_p->size; ++i) {
        const auto token_iter = token_count.find(cur_p->data[i].id);
        if (token_iter == token_count.end()) {
            continue;
        }

        const int count = token_iter->second;

        // The academic publication that described this technique actually just only divided, but that would cause tokens with negative logits to become more likely, which is obviously wrong.
        // This is common fix for this problem, which is to multiply by the penalty instead of dividing.
        if (cur_p->data[i].logit <= 0) {
            cur_p->data[i].logit *= ctx->penalty_repeat;
        } else {
            cur_p->data[i].logit /= ctx->penalty_repeat;
        }

        cur_p->data[i].logit -= float(count) * ctx->penalty_freq + float(count > 0) * ctx->penalty_present;
    }

    cur_p->sorted = false;

    if (!ctx->penalize_nl && nl_found) {
        // restore the logit of the newline token if it was penalized
        cur_p->data[nl_idx].logit = nl_logit;
    }
}

static void llama_sampler_penalties_reset(struct llama_sampler * smpl) {
    auto * ctx = (llama_sampler_penalties *) smpl->ctx;
    ctx->prev.clear();
}

static struct llama_sampler * llama_sampler_penalties_clone(const struct llama_sampler * smpl) {
    const auto * ctx = (const llama_sampler_penalties *) smpl->ctx;
    auto * result = llama_sampler_init_penalties(
            ctx->n_vocab,
            ctx->special_eos_id,
            ctx->linefeed_id,
            ctx->penalty_last_n,
            ctx->penalty_repeat,
            ctx->penalty_freq,
            ctx->penalty_present,
            ctx->penalize_nl,
            ctx->ignore_eos);

    // copy the state
    {
        auto * result_ctx = (llama_sampler_penalties *) result->ctx;

        result_ctx->prev = ctx->prev;
    }

    return result;
}

static void llama_sampler_penalties_free(struct llama_sampler * smpl) {
    delete (llama_sampler_penalties *) smpl->ctx;
}

static struct llama_sampler_i llama_sampler_penalties_i = {
    /* .name   = */ llama_sampler_penalties_name,
    /* .accept = */ llama_sampler_penalties_accept,
    /* .apply  = */ llama_sampler_penalties_apply,
    /* .reset  = */ llama_sampler_penalties_reset,
    /* .clone  = */ llama_sampler_penalties_clone,
    /* .free   = */ llama_sampler_penalties_free,
};

struct llama_sampler * llama_sampler_init_penalties(
        int32_t n_vocab,
        llama_token special_eos_id,
        llama_token linefeed_id,
        int32_t penalty_last_n,
        float penalty_repeat,
        float penalty_freq,
        float penalty_present,
        bool penalize_nl,
        bool ignore_eos) {
    if (linefeed_id == LLAMA_TOKEN_NULL) {
        penalize_nl = true;
    }

    if (special_eos_id == LLAMA_TOKEN_NULL) {
        ignore_eos = false;
    }

    penalty_last_n = std::max(penalty_last_n, 0);

    return new llama_sampler {
        /* .iface = */ &llama_sampler_penalties_i,
        /* .ctx   = */ new llama_sampler_penalties {
            /* .n_vocab         = */ n_vocab,
            /* .special_eos_id  = */ special_eos_id,
            /* .linefeed_id     = */ linefeed_id,
            /* .penalty_last_n  = */ penalty_last_n,
            /* .penalty_repeat  = */ penalty_repeat,
            /* .penalty_freq    = */ penalty_freq,
            /* .penalty_present = */ penalty_present,
            /* .penalize_nl     = */ penalize_nl,
            /* .ignore_eos      = */ ignore_eos,
            /* .prev            = */ ring_buffer<llama_token>(penalty_last_n),
        },
    };
}

// logit-bias

struct llama_sampler_logit_bias {
    const int32_t n_vocab;

    const std::vector<llama_logit_bias> logit_bias;

    std::vector<llama_logit_bias> to_search;
};

static const char * llama_sampler_logit_bias_name(const struct llama_sampler * /*smpl*/) {
    return "logit-bias";
}

static void llama_sampler_logit_bias_apply(struct llama_sampler * smpl, llama_token_data_array * cur_p) {
    auto * ctx = (llama_sampler_logit_bias *) smpl->ctx;

    if (ctx->logit_bias.empty()) {
        return;
    }

    ctx->to_search.clear();

    // update the candidates that have not been shuffled in the vocabulary (i.e. idx == id)
    for (const auto & lb : ctx->logit_bias) {
        if (lb.token >= 0 && cur_p->size > (size_t) lb.token && cur_p->data[lb.token].id == lb.token) {
            cur_p->data[lb.token].logit += lb.bias;
        } else {
            ctx->to_search.push_back(lb);
        }
    }

    if (ctx->to_search.empty()) {
        return;
    }

    // search for the remaining candidates that were not found in the previous step
    for (size_t i = 0; i < cur_p->size; ++i) {
        for (const auto & lb : ctx->to_search) {
            if (cur_p->data[i].id == lb.token) {
                cur_p->data[i].logit += lb.bias;
                break;
            }
        }
    }
}

static struct llama_sampler * llama_sampler_logit_bias_clone(const struct llama_sampler * smpl) {
    const auto * ctx = (const llama_sampler_logit_bias *) smpl->ctx;
    return llama_sampler_init_logit_bias(ctx->n_vocab, ctx->logit_bias.size(), ctx->logit_bias.data());
}

static void llama_sampler_logit_bias_free(struct llama_sampler * smpl) {
    delete (llama_sampler_logit_bias *) smpl->ctx;
}

static struct llama_sampler_i llama_sampler_logit_bias_i = {
    /* .name   = */ llama_sampler_logit_bias_name,
    /* .accept = */ nullptr,
    /* .apply  = */ llama_sampler_logit_bias_apply,
    /* .reset  = */ nullptr,
    /* .clone  = */ llama_sampler_logit_bias_clone,
    /* .free   = */ llama_sampler_logit_bias_free,
};

struct llama_sampler * llama_sampler_init_logit_bias(
                         int32_t   n_vocab,
                         int32_t   n_logit_bias,
          const llama_logit_bias * logit_bias) {
    return new llama_sampler {
        /* .iface = */ &llama_sampler_logit_bias_i,
        /* .ctx   = */ new llama_sampler_logit_bias {
            /* .n_vocab    = */ n_vocab,
            /* .logit_bias = */ std::vector<llama_logit_bias>(logit_bias, logit_bias + n_logit_bias),
            /* .to_search  = */ {},
        },
    };
}

// utils

uint32_t llama_sampler_get_seed(const struct llama_sampler * smpl) {
    if (smpl->iface == &llama_sampler_dist_i) {
        return ((const llama_sampler_dist *) smpl->ctx)->seed_cur;
    }

    if (smpl->iface == &llama_sampler_mirostat_i) {
        return ((const llama_sampler_mirostat *) smpl->ctx)->seed_cur;
    }

    if (smpl->iface == &llama_sampler_mirostat_v2_i) {
        return ((const llama_sampler_mirostat_v2 *) smpl->ctx)->seed_cur;
    }

    if (smpl->iface == &llama_sampler_chain_i) {
        const auto * ctx = (const llama_sampler_chain *) smpl->ctx;
        for (auto it = ctx->samplers.rbegin(); it != ctx->samplers.rend(); ++it) {
            const uint32_t seed = llama_sampler_get_seed(*it);
            if (seed != LLAMA_DEFAULT_SEED) {
                return seed;
            }
        }
    }

    return LLAMA_DEFAULT_SEED;
}

// perf

struct llama_perf_sampler_data llama_perf_sampler(const struct llama_sampler * chain) {
    struct llama_perf_sampler_data data = {};

    if (chain == nullptr || chain->iface != &llama_sampler_chain_i) {
        GGML_ABORT("%s: invalid sampler passed - requires a sampler created with llama_sampler_chain_init()\n", __func__);
    }

    const auto * ctx = (const struct llama_sampler_chain *) chain->ctx;

    data.t_sample_ms = 1e-3 * ctx->t_sample_us;
    data.n_sample    = std::max(0, ctx->n_sample);

    return data;
}

void llama_perf_sampler_print(const struct llama_sampler * chain) {
    const auto data = llama_perf_sampler(chain);

    LLAMA_LOG_INFO("%s:    sampling time = %10.2f ms / %5d runs   (%8.2f ms per token, %8.2f tokens per second)\n",
            __func__, data.t_sample_ms, data.n_sample, data.t_sample_ms / data.n_sample, 1e3 / data.t_sample_ms * data.n_sample);
}

void llama_perf_sampler_reset(struct llama_sampler * chain) {
    if (chain == nullptr || chain->iface != &llama_sampler_chain_i) {
        GGML_ABORT("%s: invalid sampler passed - requires a sampler created with llama_sampler_chain_init()\n", __func__);
    }

    auto * ctx = (struct llama_sampler_chain *) chain->ctx;

    ctx->t_sample_us = ctx->n_sample = 0;
}
