#pragma once
#include "common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t ggml_backend_amx_get_alloc_size(const struct ggml_tensor * tensor);

void ggml_backend_amx_convert_weight(struct ggml_tensor * tensor, const void * data, size_t offset, size_t size);

void ggml_backend_amx_mul_mat(ggml_backend_amx_context * ctx, struct ggml_tensor * dst);

#ifdef __cplusplus
}
#endif
