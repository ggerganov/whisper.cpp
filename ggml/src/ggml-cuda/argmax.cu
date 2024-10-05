#include "common.cuh"
#include "argmax.cuh"
#include "sum.cuh"

#include <cstdint>

static __global__ void argmax_f32(
    const float * x, int32_t * dst, const int64_t ncols, const int64_t nrows) {

    int argmax_thread = 0;
    const int64_t row0 = (int64_t)blockIdx.x*WARP_SIZE;

#pragma unroll
    for (int64_t row1 = 0; row1 < WARP_SIZE; ++row1) {
        const int64_t row = row0 + row1;

        if (row >= nrows) {
            break;
        }

        float maxval = -FLT_MAX;
        int   argmax = -1;

        for (int32_t col = threadIdx.x; col < ncols; col += WARP_SIZE) {
            const float val        = x[row*ncols + col];
            const int   bigger     = val > maxval;
            const int   not_bigger = bigger ^ 0x00000001;

            maxval = maxval*not_bigger + val*bigger;
            argmax = argmax*not_bigger + col*bigger;
        }

#pragma unroll
        for (int mask = 16; mask > 0; mask >>= 1) {
            const float val        = __shfl_xor_sync(0xFFFFFFFF, maxval, mask, WARP_SIZE);
            const int   col        = __shfl_xor_sync(0xFFFFFFFF, argmax, mask, WARP_SIZE);
            const int   bigger     = val > maxval;
            const int   not_bigger = bigger ^ 0x00000001;

            maxval = maxval*not_bigger + val*bigger;
            argmax = argmax*not_bigger + col*bigger;
        }

        const int store = row1 == threadIdx.x;
        argmax_thread += store*argmax;
    }

    const int row = row0 + threadIdx.x;

    if (row >= nrows) {
        return;
    }

    dst[row] = argmax_thread;
}

void ggml_cuda_argmax(ggml_backend_cuda_context & ctx, ggml_tensor * dst) {
    const ggml_tensor * src0 = dst->src[0];

    GGML_ASSERT(src0->type == GGML_TYPE_F32);
    GGML_ASSERT( dst->type == GGML_TYPE_I32);

    GGML_ASSERT(ggml_is_contiguous(src0));

    const int64_t ne00  = src0->ne[0];
    const int64_t nrows = ggml_nrows(src0);

    const float * src0_d = (const float *) src0->data;
    int32_t     * dst_d  = (int32_t     *) dst->data;

    cudaStream_t stream = ctx.stream();

    const int64_t num_blocks = (nrows + WARP_SIZE - 1) / WARP_SIZE;

    const dim3 blocks_dim(WARP_SIZE, 1, 1);
    const dim3 blocks_num(num_blocks, 1, 1);

    argmax_f32<<<blocks_num, blocks_dim, 0, stream>>>(src0_d, dst_d, ne00, nrows);
}
