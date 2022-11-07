#pragma once

#include <stdint.h>
#include <stddef.h>

// TODO: this will hold dynamic context data in the future
//       currently unused
struct ggml_mtl_context {
    void * dummy;
};

struct ggml_mtl_object {
    int32_t id;
    void * data;
};

struct ggml_mtl_context * ggml_mtl_init(void);

struct ggml_mtl_object ggml_mtl_alloc(size_t size);

// multiply matrix by vector
void ggml_mtl_mul_mat_vec_f16(
    struct ggml_mtl_context * ctx,
    struct ggml_mtl_object    src0,  // matrix f16
    const __fp16            * src1,  // vector f16
    float                   * dst,   // vector f32
    int                       nrows,
    int                       ncols);

// multiply matrix by matrix
void ggml_mtl_mul_mat_f16(
    struct ggml_mtl_context * ctx,
    struct ggml_mtl_object    src0,  // matrix f16
    const __fp16            * src1,  // matrix f16
    float                   * dst,   // matrix f32
    int                       nrows0,
    int                       nrows1,
    int                       ncols);
