
#include "whisper-encoder.h"

#include <ane.h>
#include <stdlib.h>
#include "data/anec_coreml_encoder_tiny.h"

#if __cplusplus
extern "C" {
#endif

struct whisper_coreml_context * whisper_coreml_init(const char * path_model) {
    struct ane_nn *nn = ane_init_coreml_encoder_tiny();
    if (nn == NULL) {
        return NULL;
    }
    struct whisper_coreml_context *ctx = malloc(sizeof(struct whisper_coreml_context));
    ctx->data = nn;
    return ctx;
}

void whisper_coreml_free(struct whisper_coreml_context * ctx) {
    ane_free((struct ane_nn *)ctx->data);
    free(ctx);
}

void whisper_coreml_encode(
        const struct whisper_coreml_context * ctx,
                                ggml_fp16_t * mel,
                                ggml_fp16_t * out) {
    ane_tile_send((struct ane_nn *)ctx->data, mel, 0);
    ane_exec((struct ane_nn *)ctx->data);
    ane_tile_read((struct ane_nn *)ctx->data, out, 0);
}

#if __cplusplus
}
#endif
