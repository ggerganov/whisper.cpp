#include <stdio.h>
#include "whisper_java.h"

struct whisper_full_params default_params;
struct whisper_context * whisper_ctx = nullptr;

struct void whisper_java_default_params(enum whisper_sampling_strategy strategy) {
    default_params = whisper_full_default_params(strategy);

//    struct whisper_java_params result = {};
//    return result;
    return;
}

void whisper_java_init_from_file(const char * path_model) {
    whisper_ctx = whisper_init_from_file(path_model);
    if (0 == default_params.n_threads) {
        whisper_java_default_params(WHISPER_SAMPLING_GREEDY);
    }
}

/** Delegates to whisper_full, but without having to pass `whisper_full_params` */
int whisper_java_full(
          struct whisper_context * ctx,
//      struct whisper_java_params   params,
                     const float * samples,
                             int   n_samples) {
    return whisper_full(ctx, default_params, samples, n_samples);
}

void whisper_java_free() {
//    free(default_params);
}
