/*
 * Copyright (c) 2023-2024 The ggml authors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "aclnn_ops.h"

#include <aclnnop/aclnn_avgpool2d.h>
#include <aclnnop/aclnn_cast.h>
#include <aclnnop/aclnn_constant_pad_nd.h>
#include <aclnnop/aclnn_copy.h>
#include <aclnnop/aclnn_cos.h>
#include <aclnnop/aclnn_exp.h>
#include <aclnnop/aclnn_fill_scalar.h>
#include <aclnnop/aclnn_group_norm.h>
#include <aclnnop/aclnn_index_fill_tensor.h>
#include <aclnnop/aclnn_layer_norm.h>
#include <aclnnop/aclnn_matmul.h>
#include <aclnnop/aclnn_max_pool.h>
#include <aclnnop/aclnn_permute.h>
#include <aclnnop/aclnn_pow_tensor_tensor.h>
#include <aclnnop/aclnn_reduce_sum.h>
#include <aclnnop/aclnn_repeat.h>
#include <aclnnop/aclnn_repeat_interleave.h>
#include <aclnnop/aclnn_roll.h>
#include <aclnnop/aclnn_sin.h>
#include <aclnnop/aclnn_softmax.h>
#include <aclnnop/aclnn_tril.h>
#include <aclnnop/aclnn_triu.h>
#include <aclnnop/aclnn_upsample_nearest_2d.h>
#include <aclnnop/aclnn_weight_quant_batch_matmul_v2.h>
#include <float.h>

#include <cmath>
#include <cstring>
#include <exception>
#include <vector>

#include "kernels/ascendc_kernels.h"

#define GGML_COMMON_DECL_C

#include "../ggml-common.h"

/**
 * @brief Repeats elements of a tensor along each dimension according to the
 * specified repeat array.
 *
 * @param ctx The context for the CANN backend operations.
 * @param acl_src The source tensor to be repeated.
 * @param acl_dst The destination tensor after repeating.
 * @param repeat_array The array specifying the number of repetitions along each
 * dimension.
 */
static void aclnn_repeat(ggml_backend_cann_context& ctx, aclTensor* acl_src,
                         aclTensor* acl_dst, int64_t* repeat_array) {
    // repeat tensor along each dim with repeat_array
    aclIntArray* repeats = aclCreateIntArray(repeat_array, GGML_MAX_DIMS);

    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    ACL_CHECK(aclnnRepeatGetWorkspaceSize(acl_src, repeats, acl_dst,
                                          &workspaceSize, &executor));

    if (workspaceSize > 0) {
        // Memory from allocator will "free" immediately, and this memory
        // will be alloced to other pointers, but it won't access before
        // this async task end because all tasks in same stream will execute
        // in queue.
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }
    ACL_CHECK(
        aclnnRepeat(workspaceAddr, workspaceSize, executor, ctx.stream()));
    ACL_CHECK(aclDestroyIntArray(repeats));
}

void ggml_cann_repeat(ggml_backend_cann_context& ctx, ggml_tensor* dst) {
    ggml_tensor* src = dst->src[0];
    GGML_ASSERT(ggml_can_repeat(src, dst));

    aclTensor* acl_src = ggml_cann_create_tensor(src);
    aclTensor* acl_dst = ggml_cann_create_tensor(dst);

    int64_t repeatsArray[] = {dst->ne[3] / src->ne[3], dst->ne[2] / src->ne[2],
                              dst->ne[1] / src->ne[1], dst->ne[0] / src->ne[0]};

    aclnn_repeat(ctx, acl_src, acl_dst, repeatsArray);
    ACL_CHECK(aclDestroyTensor(acl_src));
    ACL_CHECK(aclDestroyTensor(acl_dst));
}

/**
 * @brief Adds two tensors element-wise and stores the result in a destination
 * tensor.
 *
 * This function performs the operation:
 * \f[
 *    dst = acl\_src0 + alpha \times acl\_src1
 * \f]
 * where alpha is a scalar value and defaults to 1.0f.
 *
 * @param ctx The context for the CANN backend operations.
 * @param acl_src0 The first source tensor.
 * @param acl_src1 The second source tensor.
 * @param acl_dst The destination tensor where the result will be stored.
 */
static void aclnn_add(ggml_backend_cann_context& ctx, aclTensor* acl_src0,
                      aclTensor* acl_src1, aclTensor* acl_dst) {
    aclScalar* alpha = nullptr;
    float alphaValue = 1.0f;
    alpha = aclCreateScalar(&alphaValue, aclDataType::ACL_FLOAT);

    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    ACL_CHECK(aclnnAddGetWorkspaceSize(acl_src0, acl_src1, alpha, acl_dst,
                                       &workspaceSize, &executor));
    if (workspaceSize > 0) {
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }

    ACL_CHECK(aclnnAdd(workspaceAddr, workspaceSize, executor, ctx.stream()));

    ACL_CHECK(aclDestroyScalar(alpha));
}

void ggml_cann_add(ggml_backend_cann_context& ctx, ggml_tensor* dst) {
    ggml_tensor* src0 = dst->src[0];
    ggml_tensor* src1 = dst->src[1];
    GGML_ASSERT(ggml_can_repeat(src1, src0) && ggml_are_same_shape(src0, dst));

    aclTensor* acl_src0;
    aclTensor* acl_src1;
    aclTensor* acl_dst;

    // Need bcast
    if (!ggml_are_same_shape(src0, src1) && ggml_cann_need_bcast(src0, src1)) {
        BCAST_SHAPE(src0, src1)
        acl_src0 = ggml_cann_create_tensor(src0, BCAST_PARAM(src0));
        acl_src1 = ggml_cann_create_tensor(src1, BCAST_PARAM(src1));
        acl_dst = ggml_cann_create_tensor(dst, BCAST_PARAM(src0));
    } else {
        acl_src0 = ggml_cann_create_tensor(src0);
        acl_src1 = ggml_cann_create_tensor(src1);
        acl_dst = ggml_cann_create_tensor(dst);
    }

    aclnn_add(ctx, acl_src0, acl_src1, acl_dst);

    ACL_CHECK(aclDestroyTensor(acl_src0));
    ACL_CHECK(aclDestroyTensor(acl_src1));
    ACL_CHECK(aclDestroyTensor(acl_dst));
}

void ggml_cann_leaky_relu(ggml_backend_cann_context& ctx, ggml_tensor* dst) {
    ggml_tensor* src = dst->src[0];

    GGML_ASSERT(src->type == GGML_TYPE_F32);
    GGML_ASSERT(dst->type == GGML_TYPE_F32);

    aclTensor* acl_src = ggml_cann_create_tensor(src);
    aclTensor* acl_dst = ggml_cann_create_tensor(dst);

    float negative_slope;
    memcpy(&negative_slope, dst->op_params, sizeof(float));
    aclScalar* acl_negative_slope =
        aclCreateScalar(&negative_slope, aclDataType::ACL_FLOAT);

    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    ACL_CHECK(aclnnLeakyReluGetWorkspaceSize(
        acl_src, acl_negative_slope, acl_dst, &workspaceSize, &executor));
    if (workspaceSize > 0) {
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }

    ACL_CHECK(
        aclnnLeakyRelu(workspaceAddr, workspaceSize, executor, ctx.stream()));

    ACL_CHECK(aclDestroyScalar(acl_negative_slope));
    ACL_CHECK(aclDestroyTensor(acl_src));
    ACL_CHECK(aclDestroyTensor(acl_dst));
}

/**
 * @brief Concatenates a list of tensors along a specified dimension and stores
 * the result in a destination tensor.
 *
 * @param ctx The context for the CANN backend operations.
 * @param tensorList The list of tensors to be concatenated.
 * @param acl_dst The destination tensor where the concatenated result will be
 * stored.
 * @param concat_dim The dimension along which the tensors will be concatenated.
 */
static void aclnn_concat(ggml_backend_cann_context& ctx,
                         aclTensorList* tensorList, aclTensor* acl_dst,
                         int64_t concat_dim) {
    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    ACL_CHECK(aclnnCatGetWorkspaceSize(tensorList, concat_dim, acl_dst,
                                       &workspaceSize, &executor));
    if (workspaceSize > 0) {
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }

    ACL_CHECK(aclnnCat(workspaceAddr, workspaceSize, executor, ctx.stream()));
}

void ggml_cann_concat(ggml_backend_cann_context& ctx, ggml_tensor* dst) {
    ggml_tensor* src0 = dst->src[0];
    ggml_tensor* src1 = dst->src[1];
    aclTensor* acl_src0 = ggml_cann_create_tensor(src0);
    aclTensor* acl_src1 = ggml_cann_create_tensor(src1);
    aclTensor* acl_dst = ggml_cann_create_tensor(dst);

    int64_t concat_dim = 1;
    aclTensor* tensors[] = {acl_src0, acl_src1};
    aclTensorList* tensorList = aclCreateTensorList(tensors, 2);
    aclnn_concat(ctx, tensorList, acl_dst, concat_dim);

    ACL_CHECK(aclDestroyTensorList(tensorList));
    ACL_CHECK(aclDestroyTensor(acl_dst));
}

/**
 * @brief Creates a tensor with values starting from `start`, incremented by
 * `step`, and ending before `stop`.
 *
 * This function performs the operation:
 * \f[
 *    \text {out }_{i+1}=\text {out }_i+\text {step}
 * \f]
 * the range is [start, stop).
 *
 * @param ctx The context for the CANN backend operations.
 * @param acl_dst The destination tensor where the values will be stored.
 * @param start The starting value of the range.
 * @param stop The ending value of the range (exclusive).
 * @param step The step size between consecutive values.
 * @param n_elements The number of elements in the destination tensor.
 */
static void aclnn_arange(ggml_backend_cann_context& ctx, aclTensor* acl_dst,
                         float start, float stop, float step,
                         int64_t n_elements) {
    int64_t steps = (int64_t)std::ceil((stop - start) / step);
    GGML_ASSERT(n_elements == steps);

    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    aclScalar* acl_start = aclCreateScalar(&start, aclDataType::ACL_FLOAT);
    aclScalar* acl_end = aclCreateScalar(&stop, aclDataType::ACL_FLOAT);
    aclScalar* acl_step = aclCreateScalar(&step, aclDataType::ACL_FLOAT);

    ACL_CHECK(aclnnArangeGetWorkspaceSize(acl_start, acl_end, acl_step, acl_dst,
                                          &workspaceSize, &executor));
    if (workspaceSize > 0) {
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }

    ACL_CHECK(
        aclnnArange(workspaceAddr, workspaceSize, executor, ctx.stream()));

    ACL_CHECK(aclDestroyScalar(acl_start));
    ACL_CHECK(aclDestroyScalar(acl_end));
    ACL_CHECK(aclDestroyScalar(acl_step));
}

void ggml_cann_arange(ggml_backend_cann_context& ctx, ggml_tensor* dst) {
    GGML_ASSERT(dst->type == GGML_TYPE_F32);

    aclTensor* acl_dst = ggml_cann_create_tensor(dst);

    int64_t n_elements = ggml_nelements(dst);
    float start;
    float stop;
    float step;
    memcpy(&start, (float*)dst->op_params + 0, sizeof(float));
    memcpy(&stop, (float*)dst->op_params + 1, sizeof(float));
    memcpy(&step, (float*)dst->op_params + 2, sizeof(float));

    aclnn_arange(ctx, acl_dst, start, stop, step, n_elements);
    ACL_CHECK(aclDestroyTensor(acl_dst));
}

void ggml_cann_sqr(ggml_backend_cann_context& ctx, ggml_tensor* dst) {
    dst->src[1] = dst->src[0];
    ggml_cann_mul_div<aclnnMulGetWorkspaceSize, aclnnMul>(ctx, dst);
}

void ggml_cann_clamp(ggml_backend_cann_context& ctx, ggml_tensor* dst) {
    ggml_tensor* src = dst->src[0];
    GGML_ASSERT(src->type == GGML_TYPE_F32);
    GGML_ASSERT(dst->type == GGML_TYPE_F32);

    float min;
    float max;
    memcpy(&min, dst->op_params, sizeof(float));
    memcpy(&max, (float*)dst->op_params + 1, sizeof(float));

    aclTensor* acl_src = ggml_cann_create_tensor(src);
    aclTensor* acl_dst = ggml_cann_create_tensor(dst);

    aclScalar* acl_min = aclCreateScalar(&min, aclDataType::ACL_FLOAT);
    aclScalar* acl_max = aclCreateScalar(&max, aclDataType::ACL_FLOAT);

    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    ACL_CHECK(aclnnClampGetWorkspaceSize(acl_src, acl_min, acl_max, acl_dst,
                                         &workspaceSize, &executor));
    if (workspaceSize > 0) {
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }

    ACL_CHECK(aclnnClamp(workspaceAddr, workspaceSize, executor, ctx.stream()));

    ACL_CHECK(aclDestroyScalar(acl_min));
    ACL_CHECK(aclDestroyScalar(acl_max));
    ACL_CHECK(aclDestroyTensor(acl_src));
    ACL_CHECK(aclDestroyTensor(acl_dst));
}

void ggml_cann_scale(ggml_backend_cann_context& ctx, ggml_tensor* dst) {
    ggml_tensor* src = dst->src[0];

    // scale factor
    float v;
    memcpy(&v, dst->op_params, sizeof(float));

    aclScalar* scale = aclCreateScalar(&v, aclDataType::ACL_FLOAT);
    aclTensor* acl_src = ggml_cann_create_tensor(src);
    aclTensor* acl_dst = ggml_cann_create_tensor(dst);

    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    ACL_CHECK(aclnnMulsGetWorkspaceSize(acl_src, scale, acl_dst, &workspaceSize,
                                        &executor));
    if (workspaceSize > 0) {
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }

    ACL_CHECK(aclnnMuls(workspaceAddr, workspaceSize, executor, ctx.stream()));

    ACL_CHECK(aclDestroyScalar(scale));
    ACL_CHECK(aclDestroyTensor(acl_src));
    ACL_CHECK(aclDestroyTensor(acl_dst));
}

void ggml_cann_argsort(ggml_backend_cann_context& ctx, ggml_tensor* dst) {
    ggml_tensor* src = dst->src[0];
    enum ggml_sort_order order = (enum ggml_sort_order)dst->op_params[0];

    aclTensor* acl_src = ggml_cann_create_tensor(src);
    aclTensor* acl_dst = ggml_cann_create_tensor(dst);
    ggml_cann_pool_alloc temp_buffer_allocator(
        ctx.pool(), ggml_nelements(dst) * sizeof(int64_t));
    void* buffer = temp_buffer_allocator.get();
    aclTensor* tmp_tensor =
        ggml_cann_create_tensor(buffer, ACL_INT64, ggml_type_size(dst->type),
                                dst->ne, dst->nb, GGML_MAX_DIMS);

    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    ACL_CHECK(aclnnArgsortGetWorkspaceSize(
        acl_src, -1, (order == GGML_SORT_ORDER_DESC ? true : false), tmp_tensor,
        &workspaceSize, &executor));
    if (workspaceSize > 0) {
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }

    ACL_CHECK(
        aclnnArgsort(workspaceAddr, workspaceSize, executor, ctx.stream()));

    workspaceSize = 0;
    ACL_CHECK(aclnnCastGetWorkspaceSize(tmp_tensor,
                                        ggml_cann_type_mapping(dst->type),
                                        acl_dst, &workspaceSize, &executor));
    if (workspaceSize > 0) {
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }

    ACL_CHECK(aclnnCast(workspaceAddr, workspaceSize, executor, ctx.stream()));

    ACL_CHECK(aclDestroyTensor(acl_src));
    ACL_CHECK(aclDestroyTensor(tmp_tensor));
    ACL_CHECK(aclDestroyTensor(acl_dst));
}

void ggml_cann_norm(ggml_backend_cann_context& ctx, ggml_tensor* dst) {
    ggml_tensor* src = dst->src[0];

    aclTensor* acl_src = ggml_cann_create_tensor(src);
    aclTensor* acl_dst = ggml_cann_create_tensor(dst);

    float eps;
    memcpy(&eps, dst->op_params, sizeof(float));

    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    std::vector<int64_t> normData = {dst->ne[0]};
    aclIntArray* norm = aclCreateIntArray(normData.data(), normData.size());
    ACL_CHECK(aclnnLayerNormGetWorkspaceSize(acl_src, norm, nullptr, nullptr,
                                             eps, acl_dst, nullptr, nullptr,
                                             &workspaceSize, &executor));

    if (workspaceSize > 0) {
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }

    ACL_CHECK(
        aclnnLayerNorm(workspaceAddr, workspaceSize, executor, ctx.stream()));

    ACL_CHECK(aclDestroyIntArray(norm));
    ACL_CHECK(aclDestroyTensor(acl_src));
    ACL_CHECK(aclDestroyTensor(acl_dst));
}

void ggml_cann_group_norm(ggml_backend_cann_context& ctx, ggml_tensor* dst) {
    ggml_tensor* src = dst->src[0];

    aclTensor* acl_src = ggml_cann_create_tensor(src);
    aclTensor* acl_dst = ggml_cann_create_tensor(dst);

    int n_groups = dst->op_params[0];

    float eps;
    memcpy(&eps, dst->op_params + 1, sizeof(float));

    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    int64_t N = src->ne[3];
    int64_t C = src->ne[2];
    int64_t HxW = src->ne[1] * src->ne[0];

    size_t type_size = ggml_type_size(src->type);
    int64_t ne[] = {n_groups, N};
    size_t nb[] = {type_size, type_size * n_groups};
    size_t n_bytes = N * n_groups;

    ggml_cann_pool_alloc temp_buffer_allocator(ctx.pool(), n_bytes * 2);
    void* buffer = temp_buffer_allocator.get();
    aclTensor* acl_mean_out = ggml_cann_create_tensor(
        buffer, ACL_FLOAT, type_size, ne, nb, ACL_FORMAT_ND);
    aclTensor* acl_rstd_out = ggml_cann_create_tensor(
        (char*)buffer + n_bytes, ACL_FLOAT, type_size, ne, nb, ACL_FORMAT_ND);

    ACL_CHECK(aclnnGroupNormGetWorkspaceSize(
        acl_src, nullptr, nullptr, N, C, HxW, n_groups, eps, acl_dst,
        acl_mean_out, acl_rstd_out, &workspaceSize, &executor));

    if (workspaceSize > 0) {
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }

    ACL_CHECK(
        aclnnGroupNorm(workspaceAddr, workspaceSize, executor, ctx.stream()));

    ACL_CHECK(aclDestroyTensor(acl_src));
    ACL_CHECK(aclDestroyTensor(acl_dst));
    ACL_CHECK(aclDestroyTensor(acl_mean_out));
    ACL_CHECK(aclDestroyTensor(acl_rstd_out));
}

void ggml_cann_acc(ggml_backend_cann_context& ctx, ggml_tensor* dst) {
    ggml_tensor* src0 = dst->src[0];
    ggml_tensor* src1 = dst->src[1];

    size_t nb1 = ((int32_t*)dst->op_params)[0];
    size_t nb2 = ((int32_t*)dst->op_params)[1];
    size_t nb3 = ((int32_t*)dst->op_params)[2];
    size_t offset = ((int32_t*)dst->op_params)[3];
    bool inplace = (bool)((int32_t*)dst->op_params)[4];

    size_t param_nb[] = {ggml_element_size(src0), nb1, nb2, nb3};

    aclTensor* acl_dst = ggml_cann_create_tensor(
        dst, src1->ne, param_nb, GGML_MAX_DIMS, ACL_FORMAT_ND, offset);
    aclTensor* acl_src1 = ggml_cann_create_tensor(src1);

    aclScalar* alpha = nullptr;
    float alphaValue = 1.0f;
    alpha = aclCreateScalar(&alphaValue, aclDataType::ACL_FLOAT);

    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    if (!inplace) {
        size_t cpy_size = ggml_nbytes(dst);
        ACL_CHECK(aclrtMemcpyAsync(dst->data, cpy_size, src0->data, cpy_size,
                                   ACL_MEMCPY_DEVICE_TO_DEVICE, ctx.stream()));
        aclTensor* acl_src0 = ggml_cann_create_tensor(
            src0, src1->ne, src0->nb, GGML_MAX_DIMS, ACL_FORMAT_ND, offset);
        ACL_CHECK(aclnnAddGetWorkspaceSize(acl_src0, acl_src1, alpha, acl_dst,
                                           &workspaceSize, &executor));
        if (workspaceSize > 0) {
            ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
            workspaceAddr = workspace_allocator.get();
        }
        ACL_CHECK(
            aclnnAdd(workspaceAddr, workspaceSize, executor, ctx.stream()));
        ACL_CHECK(aclDestroyTensor(acl_src0));
    } else {
        ACL_CHECK(aclnnInplaceAddGetWorkspaceSize(acl_dst, acl_src1, alpha,
                                                  &workspaceSize, &executor));
        if (workspaceSize > 0) {
            ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
            workspaceAddr = workspace_allocator.get();
        }
        ACL_CHECK(aclnnInplaceAdd(workspaceAddr, workspaceSize, executor,
                                  ctx.stream()));
    }

    ACL_CHECK(aclDestroyTensor(acl_src1));
    ACL_CHECK(aclDestroyTensor(acl_dst));
}

void ggml_cann_sum_rows(ggml_backend_cann_context& ctx, ggml_tensor* dst) {
    ggml_tensor* src = dst->src[0];

    aclTensor* acl_src = ggml_cann_create_tensor(src);

    GGML_ASSERT(dst->ne[0] == 1);
    aclTensor* acl_dst = ggml_cann_create_tensor(dst);

    int64_t reduce_dims_host[] = {3};
    aclIntArray* reduce_dims = aclCreateIntArray(reduce_dims_host, 1);

    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    ACL_CHECK(aclnnReduceSumGetWorkspaceSize(
        acl_src, reduce_dims, true, ggml_cann_type_mapping(src->type), acl_dst,
        &workspaceSize, &executor));
    if (workspaceSize > 0) {
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }

    ACL_CHECK(
        aclnnReduceSum(workspaceAddr, workspaceSize, executor, ctx.stream()));

    ACL_CHECK(aclDestroyTensor(acl_src));
    ACL_CHECK(aclDestroyTensor(acl_dst));
}

void ggml_cann_upsample_nearest2d(ggml_backend_cann_context& ctx,
                                  ggml_tensor* dst) {
    ggml_tensor* src = dst->src[0];
    aclTensor* acl_src =
        ggml_cann_create_tensor(src, nullptr, nullptr, 0, ACL_FORMAT_NCHW);
    aclTensor* acl_dst =
        ggml_cann_create_tensor(dst, nullptr, nullptr, 0, ACL_FORMAT_NCHW);

    std::vector<int64_t> output_size{dst->ne[1], dst->ne[0]};
    auto output_size_array = aclCreateIntArray(output_size.data(), 2);

    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    ACL_CHECK(aclnnUpsampleNearest2dGetWorkspaceSize(
        acl_src, output_size_array, acl_dst, &workspaceSize, &executor));
    if (workspaceSize > 0) {
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }

    ACL_CHECK(aclnnUpsampleNearest2d(workspaceAddr, workspaceSize, executor,
                                     ctx.stream()));

    ACL_CHECK(aclDestroyIntArray(output_size_array));
    ACL_CHECK(aclDestroyTensor(acl_src));
    ACL_CHECK(aclDestroyTensor(acl_dst));
}

/**
 * @brief Pads a tensor with a specified value along each dimension.
 *
 * This function performs padding of the source tensor `acl_src` and stores the
 * result in the destination tensor `acl_dst`. The padding values for each
 * dimension are specified in the `paddings` array.
 *
 * @param ctx The context for the CANN backend operations.
 * @param acl_src The source tensor to be padded.
 * @param acl_dst The destination tensor where the padded result will be stored.
 * @param paddings An array specifying the padding values for each dimension.
 * The size of the array should be twice the number of dimensions of the tensor.
 * @param value The value to be used for padding. The default value is 0.0.
 */
static void aclnn_pad(ggml_backend_cann_context& ctx, aclTensor* acl_src,
                      aclTensor* acl_dst, int64_t* paddings,
                      float value = 0.0f) {
    aclIntArray* acl_pad = aclCreateIntArray(paddings, GGML_MAX_DIMS * 2);
    aclScalar* acl_value = aclCreateScalar(&value, aclDataType::ACL_FLOAT);

    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    ACL_CHECK(aclnnConstantPadNdGetWorkspaceSize(
        acl_src, acl_pad, acl_value, acl_dst, &workspaceSize, &executor));

    if (workspaceSize > 0) {
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }

    ACL_CHECK(aclnnConstantPadNd(workspaceAddr, workspaceSize, executor,
                                 ctx.stream()));

    ACL_CHECK(aclDestroyIntArray(acl_pad));
    ACL_CHECK(aclDestroyScalar(acl_value));
}

void ggml_cann_pad(ggml_backend_cann_context& ctx, ggml_tensor* dst) {
    ggml_tensor* src = dst->src[0];
    aclTensor* acl_src = ggml_cann_create_tensor(src);
    aclTensor* acl_dst = ggml_cann_create_tensor(dst);

    // padding: value in the array means how much distance will be padding.
    // the position of elements in the array means which dirction to padding,
    // each position means: [dim0.front, dim0.behind, dim1.front, dim1.behind,
    //                       dim2.front, dim2.behind, dim3.front, dim3.behind]
    int64_t paddings[] = {
        0, dst->ne[0] - src->ne[0], 0, dst->ne[1] - src->ne[1],
        0, dst->ne[2] - src->ne[2], 0, dst->ne[3] - src->ne[3]};
    aclnn_pad(ctx, acl_src, acl_dst, paddings);

    ACL_CHECK(aclDestroyTensor(acl_dst));
    ACL_CHECK(aclDestroyTensor(acl_src));
}

/**
 * @brief Performs 2D average pooling on the input tensor and stores the result
 * in the destination tensor.
 *
 * This function performs average pooling on the source tensor and stores the
 * result in the destination tensor. The pooling parameters (kernel size,
 * strides, padding) are specified in the `op_params` of the destination tensor.
 *
 * @param ctx The context for the CANN backend operations.
 * @param dst The destination tensor where the result will be stored. The source
 * tensor is referenced by `dst->src[0]`.
 */
static void ggml_cann_avg_pool2d(ggml_backend_cann_context& ctx,
                                 ggml_tensor* dst) {
    ggml_tensor* src = dst->src[0];
    GGML_ASSERT(src->type == GGML_TYPE_F32);
    GGML_ASSERT(dst->type == GGML_TYPE_F32);

    aclTensor* acl_src =
        ggml_cann_create_tensor(src, nullptr, nullptr, 0, ACL_FORMAT_NCHW);
    aclTensor* acl_dst =
        ggml_cann_create_tensor(dst, nullptr, nullptr, 0, ACL_FORMAT_NCHW);

    const int32_t* opts = (const int32_t*)dst->op_params;
    const int k0 = opts[1];
    const int k1 = opts[2];
    const int s0 = opts[3];
    const int s1 = opts[4];
    const int p0 = opts[5];
    const int p1 = opts[6];

    std::vector<int64_t> kernel_dims = {k1, k0};
    std::vector<int64_t> stride_dims = {s1, s0};
    std::vector<int64_t> padding_avg_dims = {p1, p0};  // (padH, padW)

    auto* kernel_size = aclCreateIntArray(kernel_dims.data(), 2);
    auto* strides = aclCreateIntArray(stride_dims.data(), 2);
    auto* paddings_avg = aclCreateIntArray(padding_avg_dims.data(), 2);

    bool ceil_mode = false;
    bool count_include_pad = true;
    int64_t divisor_override = 0;
    int8_t cube_math_type = 0;

    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    ACL_CHECK(aclnnAvgPool2dGetWorkspaceSize(
        acl_src, kernel_size, strides, paddings_avg, ceil_mode,
        count_include_pad, divisor_override, cube_math_type, acl_dst,
        &workspaceSize, &executor));

    if (workspaceSize > 0) {
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }
    ACL_CHECK(
        aclnnAvgPool2d(workspaceAddr, workspaceSize, executor, ctx.stream()));

    ACL_CHECK(aclDestroyTensor(acl_src));
    ACL_CHECK(aclDestroyTensor(acl_dst));
    ACL_CHECK(aclDestroyIntArray(kernel_size));
    ACL_CHECK(aclDestroyIntArray(strides));
    ACL_CHECK(aclDestroyIntArray(paddings_avg));
}

/**
 * @brief Performs 2D max pooling on the input tensor and stores the result in
 * the destination tensor.
 *
 * This function performs max pooling on the source tensor and stores the result
 * in the destination tensor. The pooling parameters (kernel size, strides,
 * padding) are specified in the `op_params` of the destination tensor.
 *
 * @param ctx The context for the CANN backend operations.
 * @param dst The destination tensor where the result will be stored. The source
 * tensor is referenced by `dst->src[0]`.
 */
static void ggml_cann_max_pool2d(ggml_backend_cann_context& ctx,
                                 ggml_tensor* dst) {
    ggml_tensor* src = dst->src[0];
    GGML_ASSERT(src->type == GGML_TYPE_F32);
    GGML_ASSERT(dst->type == GGML_TYPE_F32);

    aclTensor* acl_src =
        ggml_cann_create_tensor(src, nullptr, nullptr, 0, ACL_FORMAT_NCHW);
    aclTensor* acl_dst =
        ggml_cann_create_tensor(dst, nullptr, nullptr, 0, ACL_FORMAT_NCHW);

    const int32_t* opts = (const int32_t*)dst->op_params;
    const int k0 = opts[1];
    const int k1 = opts[2];
    const int s0 = opts[3];
    const int s1 = opts[4];
    const int p0 = opts[5];
    const int p1 = opts[6];

    int64_t temp_ne[] = {src->ne[0] + p0 * 2, src->ne[1] + p1 * 2, src->ne[2],
                         src->ne[3]};
    size_t temp_nb[GGML_MAX_DIMS];

    temp_nb[0] = ggml_element_size(src);
    for (int i = 1; i < GGML_MAX_DIMS; i++) {
        temp_nb[i] = temp_nb[i - 1] * temp_ne[i - 1];
    }

    ggml_cann_pool_alloc temp_buffer_allocator(
        ctx.pool(), ggml_nbytes(src) + p0 * 2 + p1 * 2 * src->nb[1]);
    void* buffer = temp_buffer_allocator.get();
    aclTensor* tmp_tensor = ggml_cann_create_tensor(
        buffer, ACL_FLOAT, ggml_element_size(src), temp_ne, temp_nb,
        GGML_MAX_DIMS, ACL_FORMAT_NCHW);

    // pad: see padding in ggml_cann_pad()
    int64_t paddings[] = {p0, p0, p1, p1, 0, 0, 0, 0};
    float value = -FLT_MAX;
    aclnn_pad(ctx, acl_src, tmp_tensor, paddings, value);

    // max_pool
    std::vector<int64_t> kernel_dims = {k1, k0};
    std::vector<int64_t> stride_dims = {s1, s0};
    // padding_max_dims: [dim0_start, dim0_end, dim1_start, dim1_end]
    std::vector<int64_t> padding_max_dims = {0, 0, 0, 0};
    std::vector<int64_t> dilation_size = {1, 1};
    auto* kernel_size = aclCreateIntArray(kernel_dims.data(), 2);
    auto* strides = aclCreateIntArray(stride_dims.data(), 2);
    auto* paddings_max = aclCreateIntArray(padding_max_dims.data(), 4);
    auto* dilations = aclCreateIntArray(dilation_size.data(), 2);

    bool ceil_mode = false;
    int64_t auto_pads = 0;

    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    ACL_CHECK(aclnnMaxPoolGetWorkspaceSize(
        tmp_tensor, kernel_size, strides, auto_pads, paddings_max, dilations,
        ceil_mode, acl_dst, &workspaceSize, &executor));
    if (workspaceSize > 0) {
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }

    ACL_CHECK(
        aclnnMaxPool(workspaceAddr, workspaceSize, executor, ctx.stream()));

    ACL_CHECK(aclDestroyTensor(acl_src));
    ACL_CHECK(aclDestroyTensor(acl_dst));
    ACL_CHECK(aclDestroyTensor(tmp_tensor));
    ACL_CHECK(aclDestroyIntArray(kernel_size));
    ACL_CHECK(aclDestroyIntArray(strides));
    ACL_CHECK(aclDestroyIntArray(paddings_max));
    ACL_CHECK(aclDestroyIntArray(dilations));
}

void ggml_cann_pool2d(ggml_backend_cann_context& ctx, ggml_tensor* dst) {
    const int32_t* opts = (const int32_t*)dst->op_params;
    enum ggml_op_pool op = static_cast<ggml_op_pool>(opts[0]);
    switch (op) {
        case GGML_OP_POOL_AVG:
            ggml_cann_avg_pool2d(ctx, dst);
            break;
        case GGML_OP_POOL_MAX:
            ggml_cann_max_pool2d(ctx, dst);
            break;
        case GGML_OP_POOL_COUNT:
            GGML_ABORT("fatal error");
            break;
    }
}

/**
 * @brief Copies data from the source tensor to the destination tensor.
 *
 * This function copies data from the source tensor `acl_src` to the destination
 * tensor `acl_dst`.
 *
 * @param ctx The context for the CANN backend operations.
 * @param acl_src The source tensor from which data will be copied.
 * @param acl_dst The destination tensor where the data will be copied to.
 */
static void cann_copy(ggml_backend_cann_context& ctx, aclTensor* acl_src,
                      aclTensor* acl_dst) {
    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    ACL_CHECK(aclnnInplaceCopyGetWorkspaceSize(acl_dst, acl_src, &workspaceSize,
                                               &executor));

    if (workspaceSize > 0) {
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }

    ACL_CHECK(
        aclnnInplaceCopy(workspaceAddr, workspaceSize, executor, ctx.stream()));
}

void ggml_cann_dup(ggml_backend_cann_context& ctx, ggml_tensor* dst) {
    ggml_tensor* src = dst->src[0];

    aclTensor* acl_src = ggml_cann_create_tensor(src);
    aclTensor* acl_dst = ggml_cann_create_tensor(dst);

    ggml_cann_pool_alloc src_extra_allocator(ctx.pool(), sizeof(ggml_tensor));
    ggml_cann_pool_alloc dst_extra_allocator(ctx.pool(), sizeof(ggml_tensor));
    src->extra = src_extra_allocator.get();
    dst->extra = dst_extra_allocator.get();
    ACL_CHECK(aclrtMemcpyAsync(src->extra, sizeof(ggml_tensor), src,
                               sizeof(ggml_tensor), ACL_MEMCPY_HOST_TO_DEVICE,
                               ctx.stream()));
    ACL_CHECK(aclrtMemcpyAsync(dst->extra, sizeof(ggml_tensor), dst,
                               sizeof(ggml_tensor), ACL_MEMCPY_HOST_TO_DEVICE,
                               ctx.stream()));

    if ((dst->type == GGML_TYPE_F16 || dst->type == GGML_TYPE_F32) &&
        ggml_are_same_shape(src, dst)) {
        cann_copy(ctx, acl_src, acl_dst);
        ACL_CHECK(aclDestroyTensor(acl_src));
        ACL_CHECK(aclDestroyTensor(acl_dst));
        return;
    }
    // TODO: simplify
    if (src->type == GGML_TYPE_F16) {
        if (dst->type == GGML_TYPE_Q8_0) {
            aclrtlaunch_ascendc_quantize_f16_q8_0(
                24, ctx.stream(), src->data, dst->data,
                ((ggml_tensor*)src->extra)->ne, ((ggml_tensor*)src->extra)->nb,
                ((ggml_tensor*)dst->extra)->ne);
            return;
        }
        if (dst->type == GGML_TYPE_Q4_0) {
            aclrtlaunch_ascendc_quantize_f16_to_q4_0(
                24, ctx.stream(), src->data, dst->data,
                ((ggml_tensor*)src->extra)->ne, ((ggml_tensor*)src->extra)->nb,
                ((ggml_tensor*)dst->extra)->ne);
            return;
        }
        if (dst->type == GGML_TYPE_F16) {
            if (ggml_are_same_shape(src, dst)) {
                cann_copy(ctx, acl_src, acl_dst);
                ACL_CHECK(aclDestroyTensor(acl_src));
                ACL_CHECK(aclDestroyTensor(acl_dst));
                return;
            }
            if (ggml_is_contiguous(dst)) {
                const size_t src_type_size = ggml_type_size(src->type);
                if (src->nb[0] == src_type_size) {
                    // src0 is contigous on first dimension, copy by rows
                    int64_t rows_num = ggml_nrows(src);

                    aclrtlaunch_ascendc_dup_by_rows_fp16(
                        rows_num, ctx.stream(), src->data, dst->data,
                        ((ggml_tensor*)src->extra)->ne,
                        ((ggml_tensor*)src->extra)->nb,
                        ((ggml_tensor*)dst->extra)->ne,
                        ((ggml_tensor*)dst->extra)->nb);
                    return;
                }
                GGML_ABORT("fatal error");
            }
            GGML_ABORT("fatal error");
        }
        if (dst->type == GGML_TYPE_F32) {
            if (ggml_are_same_shape(src, dst)) {
                cann_copy(ctx, acl_src, acl_dst);
                ACL_CHECK(aclDestroyTensor(acl_src));
                ACL_CHECK(aclDestroyTensor(acl_dst));
                return;
            }
            if (ggml_is_contiguous(dst)) {
                const size_t src_type_size = ggml_type_size(src->type);
                if (src->nb[0] == src_type_size) {
                    // src0 is contigous on first dimension, copy by rows
                    int64_t rows_num = ggml_nrows(src);
                    aclrtlaunch_ascendc_dup_by_rows_fp16_to_fp32(
                        rows_num, ctx.stream(), src->data, dst->data,
                        ((ggml_tensor*)src->extra)->ne,
                        ((ggml_tensor*)src->extra)->nb,
                        ((ggml_tensor*)dst->extra)->ne,
                        ((ggml_tensor*)dst->extra)->nb);
                    return;
                }
                GGML_ABORT("fatal error");
            }
            GGML_ABORT("fatal error");
        }
        // TODO
        GGML_ABORT("fatal error");
    } else if (src->type == GGML_TYPE_F32) {
        // TODO: if (src0->type == dst->type && ne00 == ne0 && nb00 == type_size
        //          && nb0 == type_size)
        if (dst->type == GGML_TYPE_Q8_0) {
            aclrtlaunch_ascendc_quantize_f32_q8_0(
                24, ctx.stream(), src->data, dst->data,
                ((ggml_tensor*)src->extra)->ne, ((ggml_tensor*)src->extra)->nb,
                ((ggml_tensor*)dst->extra)->ne);
            return;
        }
        if (dst->type == GGML_TYPE_Q4_0) {
            aclrtlaunch_ascendc_quantize_f32_to_q4_0(
                24, ctx.stream(), src->data, dst->data,
                ((ggml_tensor*)src->extra)->ne, ((ggml_tensor*)src->extra)->nb,
                ((ggml_tensor*)dst->extra)->ne);
            return;
        }
        if (dst->type == GGML_TYPE_F32) {
            if (ggml_are_same_shape(src, dst)) {
                cann_copy(ctx, acl_src, acl_dst);
                ACL_CHECK(aclDestroyTensor(acl_src));
                ACL_CHECK(aclDestroyTensor(acl_dst));
                return;
            }
            if (ggml_is_contiguous(dst)) {
                const size_t src_type_size = ggml_type_size(src->type);
                if (src->nb[0] == src_type_size) {
                    // src0 is contigous on first dimension, copy by rows
                    int64_t rows_num = ggml_nrows(src);
                    aclrtlaunch_ascendc_dup_by_rows_fp32(
                        rows_num, ctx.stream(), src->data, dst->data,
                        ((ggml_tensor*)src->extra)->ne,
                        ((ggml_tensor*)src->extra)->nb,
                        ((ggml_tensor*)dst->extra)->ne,
                        ((ggml_tensor*)dst->extra)->nb);
                    return;
                }
                GGML_ABORT("fatal error");
            } else {
                // TODO: dst not contiguous
                GGML_ABORT("fatal error");
            }
        }
        if (dst->type == GGML_TYPE_F16) {
            if (ggml_are_same_shape(src, dst)) {
                cann_copy(ctx, acl_src, acl_dst);
                ACL_CHECK(aclDestroyTensor(acl_src));
                ACL_CHECK(aclDestroyTensor(acl_dst));
                return;
            }
            if (ggml_is_contiguous(dst)) {
                const size_t src_type_size = ggml_type_size(src->type);
                if (src->nb[0] == src_type_size) {
                    // src0 is contigous on first dimension, copy by rows
                    int64_t rows_num = ggml_nrows(src);
                    aclrtlaunch_ascendc_dup_by_rows_fp32_to_fp16(
                        rows_num, ctx.stream(), src->data, dst->data,
                        ((ggml_tensor*)src->extra)->ne,
                        ((ggml_tensor*)src->extra)->nb,
                        ((ggml_tensor*)dst->extra)->ne,
                        ((ggml_tensor*)dst->extra)->nb);
                    return;
                }
                GGML_ABORT("fatal error");
            }
        }
        // TODO
        GGML_ABORT("fatal error");
    } else {
        if (ggml_are_same_shape(src, dst)) {
            cann_copy(ctx, acl_src, acl_dst);
            ACL_CHECK(aclDestroyTensor(acl_src));
            ACL_CHECK(aclDestroyTensor(acl_dst));
            return;
        }
        GGML_ABORT("fatal error");
    }
}

#ifdef __cplusplus
extern "C" {
#endif
aclnnStatus aclnnRmsNormGetWorkspaceSize(const aclTensor* x,
                                         const aclTensor* gamma, double epsilon,
                                         const aclTensor* yOut,
                                         const aclTensor* rstdOout,
                                         uint64_t* workspaceSize,
                                         aclOpExecutor** executor);
aclnnStatus aclnnRmsNorm(void* workspace, uint64_t workspaceSize,
                         aclOpExecutor* executor, aclrtStream stream);
#ifdef __cplusplus
}
#endif

/**
 * @brief Creates an ACL tensor initialized with zeros using a provided buffer.
 *
 * This function initializes a tensor with zeros using the specified buffer and
 * tensor parameters.
 *
 * @param ctx The context for the CANN backend operations.
 * @param buffer The buffer to be used for the tensor data.
 * @param n_bytes The size of the buffer in bytes.
 * @param ne An array specifying the extents (sizes) of each dimension of the
 * tensor.
 * @param dims The number of dimensions of the tensor.
 * @param type The data type of the tensor.
 * @param type_size The size of each element in the tensor data type.
 * @return An ACL tensor initialized with zeros.
 */
static aclTensor* aclnn_zero(ggml_backend_cann_context& ctx, void* buffer,
                             size_t n_bytes, int64_t* ne, int64_t dims,
                             aclDataType type, size_t type_size) {
    size_t nb[GGML_MAX_DIMS];
    nb[0] = type_size;
    for (int i = 1; i < dims; i++) {
        nb[i] = nb[i - 1] * ne[i - 1];
    }

    ACL_CHECK(aclrtMemsetAsync(buffer, n_bytes, 0, n_bytes, ctx.stream()));
    aclTensor* zero =
        ggml_cann_create_tensor(buffer, type, type_size, ne, nb, dims);
    return zero;
}

/**
 * @brief Creates an ACL tensor initialized with ones using a provided buffer.
 *
 * This function initializes a tensor with ones using the specified buffer and
 * tensor parameters.
 *
 * @param ctx The context for the CANN backend operations.
 * @param buffer The buffer to be used for the tensor data.
 * @param n_bytes The size of the buffer in bytes.
 * @param ne An array specifying the extents (sizes) of each dimension of the
 * tensor.
 * @param dims The number of dimensions of the tensor.
 * @param type The data type of the tensor.
 * @param type_size The size of each element in the tensor data type.
 * @param value The value to be used for initializing the tensor (default
 * is 1.0).
 * @return An ACL tensor initialized with ones.
 */
static aclTensor* aclnn_ones(ggml_backend_cann_context& ctx, void* buffer,
                             size_t n_bytes, int64_t* ne, int64_t dims,
                             aclDataType type, size_t type_size,
                             float value = 1.0f) {
    aclTensor* acl_tensor =
        aclnn_zero(ctx, buffer, n_bytes, ne, dims, type, type_size);
    float alpha_host = 1.0f;
    aclScalar* alpha = aclCreateScalar(&alpha_host, aclDataType::ACL_FLOAT);
    aclScalar* other = aclCreateScalar(&value, aclDataType::ACL_FLOAT);

    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    ACL_CHECK(aclnnInplaceAddsGetWorkspaceSize(acl_tensor, other, alpha,
                                               &workspaceSize, &executor));

    if (workspaceSize > 0) {
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }
    ACL_CHECK(
        aclnnInplaceAdds(workspaceAddr, workspaceSize, executor, ctx.stream()));

    return acl_tensor;
}

void ggml_cann_rms_norm(ggml_backend_cann_context& ctx, ggml_tensor* dst) {
    ggml_tensor* src = dst->src[0];

    aclTensor* acl_src = ggml_cann_create_tensor(src);
    aclTensor* acl_dst = ggml_cann_create_tensor(dst);

    float eps;
    memcpy(&eps, dst->op_params, sizeof(float));

    GGML_ASSERT(eps > 0.0f);

    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    size_t one_tensor_n_bytes = src->ne[0] * ggml_element_size(src);
    ggml_cann_pool_alloc one_tensor_allocator(ctx.pool(), one_tensor_n_bytes);

    aclTensor* acl_gamma = aclnn_ones(
        ctx, one_tensor_allocator.get(), one_tensor_n_bytes, src->ne, 1,
        ggml_cann_type_mapping(src->type), ggml_element_size(src));

    size_t zero_tensor_n_bytes =
        src->ne[1] * src->ne[2] * src->ne[3] * ggml_element_size(src);
    ggml_cann_pool_alloc zero_tensor_allocator(ctx.pool(), zero_tensor_n_bytes);
    aclTensor* acl_rstd =
        aclnn_zero(ctx, zero_tensor_allocator.get(), zero_tensor_n_bytes,
                   src->ne, GGML_MAX_DIMS, ggml_cann_type_mapping(src->type),
                   ggml_element_size(src));

    ACL_CHECK(aclnnRmsNormGetWorkspaceSize(
        acl_src, acl_gamma, eps, acl_dst, acl_rstd, &workspaceSize, &executor));

    if (workspaceSize > 0) {
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }

    ACL_CHECK(
        aclnnRmsNorm(workspaceAddr, workspaceSize, executor, ctx.stream()));

    ACL_CHECK(aclDestroyTensor(acl_src));
    ACL_CHECK(aclDestroyTensor(acl_dst));
    ACL_CHECK(aclDestroyTensor(acl_gamma));
    ACL_CHECK(aclDestroyTensor(acl_rstd));
}

// TODO: performace is low.
void ggml_cann_diag_mask(ggml_backend_cann_context& ctx, ggml_tensor* dst,
                         float value) {
    ggml_tensor* src = dst->src[0];

    aclTensor* acl_src = ggml_cann_create_tensor(src);
    aclTensor* acl_dst = ggml_cann_create_tensor(dst);

    const int n_past = ((int32_t*)dst->op_params)[0];

    size_t one_tensor_n_bytes = src->ne[0] * src->ne[1] * src->ne[2] *
                                src->ne[3] * ggml_element_size(src);
    ggml_cann_pool_alloc one_tensor_allocator(ctx.pool(), one_tensor_n_bytes);

    aclTensor* mask_tensor =
        aclnn_ones(ctx, one_tensor_allocator.get(), one_tensor_n_bytes, src->ne,
                   GGML_MAX_DIMS, ggml_cann_type_mapping(src->type),
                   ggml_element_size(src), value);

    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    ACL_CHECK(aclnnInplaceTriuGetWorkspaceSize(mask_tensor, n_past + 1,
                                               &workspaceSize, &executor));
    if (workspaceSize > 0) {
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }

    ACL_CHECK(
        aclnnInplaceTriu(workspaceAddr, workspaceSize, executor, ctx.stream()));

    ACL_CHECK(aclnnTrilGetWorkspaceSize(acl_src, n_past + 1, acl_dst,
                                        &workspaceSize, &executor));
    if (workspaceSize > 0) {
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }

    ACL_CHECK(aclnnTril(workspaceAddr, workspaceSize, executor, ctx.stream()));

    aclScalar* alpha = nullptr;
    float alphaValue = 1.0f;
    alpha = aclCreateScalar(&alphaValue, aclDataType::ACL_FLOAT);

    ACL_CHECK(aclnnInplaceAddGetWorkspaceSize(acl_dst, mask_tensor, alpha,
                                              &workspaceSize, &executor));
    if (workspaceSize > 0) {
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }
    ACL_CHECK(
        aclnnInplaceAdd(workspaceAddr, workspaceSize, executor, ctx.stream()));

    ACL_CHECK(aclDestroyScalar(alpha));
    ACL_CHECK(aclDestroyTensor(mask_tensor));
    ACL_CHECK(aclDestroyTensor(acl_src));
    ACL_CHECK(aclDestroyTensor(acl_dst));
}

/**
 * @brief Casts the data type of a source tensor to a destination tensor.
 *
 * This function casts the data type of the source tensor `acl_src` to the
 * specified data type `cast_data_type` and stores the result in the destination
 * tensor `acl_dst`.
 *
 * @param ctx The context for the CANN backend operations.
 * @param acl_src The source tensor whose data type will be casted.
 * @param acl_dst The destination tensor where the casted result will be stored.
 * @param cast_data_type The target data type to which the source tensor will be
 * casted.
 */
static void aclnn_cast(ggml_backend_cann_context& ctx, aclTensor* acl_src,
                       aclTensor* acl_dst, aclDataType cast_data_type) {
    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    ACL_CHECK(aclnnCastGetWorkspaceSize(acl_src, cast_data_type, acl_dst,
                                        &workspaceSize, &executor));
    if (workspaceSize > 0) {
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }

    ACL_CHECK(aclnnCast(workspaceAddr, workspaceSize, executor, ctx.stream()));
}

/**
 * @brief Permutes the dimensions of a tensor according to a specified order.
 *
 * This function permutes the dimensions of the source tensor `acl_src`
 * according to the order specified in the `new_dim` array and stores the result
 * in the destination tensor `acl_dst`.
 *
 * @param ctx The context for the CANN backend operations.
 * @param acl_src The source tensor whose dimensions will be permuted.
 * @param acl_dst The destination tensor where the permuted result will be
 * stored.
 * @param new_dim An array specifying the new order of dimensions for the
 * tensor.
 * @param dims The number of dimensions in the tensor.
 */
static void aclnn_permute(ggml_backend_cann_context& ctx, aclTensor* acl_src,
                          aclTensor* acl_dst, int64_t* new_dim, uint64_t dims) {
    aclIntArray* acl_dims = aclCreateIntArray(new_dim, dims);

    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    ACL_CHECK(aclnnPermuteGetWorkspaceSize(acl_src, acl_dims, acl_dst,
                                           &workspaceSize, &executor));
    if (workspaceSize > 0) {
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }

    ACL_CHECK(
        aclnnPermute(workspaceAddr, workspaceSize, executor, ctx.stream()));

    ACL_CHECK(aclDestroyIntArray(acl_dims));
}

#ifdef __cplusplus
extern "C" {
#endif
aclnnStatus aclnnIm2colGetWorkspaceSize(const aclTensor* self,
                                        const aclIntArray* kernelSize,
                                        const aclIntArray* dilation,
                                        const aclIntArray* padding,
                                        const aclIntArray* stride,
                                        aclTensor* out, uint64_t* workspaceSize,
                                        aclOpExecutor** executor);
aclnnStatus aclnnIm2col(void* workspace, uint64_t workspaceSize,
                        aclOpExecutor* executor, aclrtStream stream);
#ifdef __cplusplus
}
#endif

static void ggml_cann_im2col_2d_post_process(ggml_backend_cann_context& ctx,
                                             ggml_tensor* dst,
                                             ggml_tensor* src1,
                                             aclTensor* tmp_cast_tensor,
                                             aclTensor* tmp_im2col_tensor) {
    // Permute: [N, IC * KH * KW, OW * OH] -> [N, OW * OH, IC * KH * KW]
    int64_t dst_ne[] = {dst->ne[0], dst->ne[1] * dst->ne[2], dst->ne[3]};
    size_t dst_nb[] = {dst->nb[0], dst->nb[1], dst->nb[3]};
    aclTensor* acl_dst =
        ggml_cann_create_tensor(dst, dst_ne, dst_nb, GGML_MAX_DIMS - 1);

    int64_t permute_dim[] = {0, 2, 1};
    if (src1->type != dst->type) {
        aclnn_permute(ctx, tmp_cast_tensor, acl_dst, permute_dim, 3);
    } else {
        aclnn_permute(ctx, tmp_im2col_tensor, acl_dst, permute_dim, 3);
    }

    // release
    ACL_CHECK(aclDestroyTensor(acl_dst));
}

static void ggml_cann_im2col_1d_post_process(
    ggml_backend_cann_context& ctx, ggml_tensor* dst, ggml_tensor* src1,
    aclTensor* tmp_cast_tensor, aclTensor* tmp_im2col_tensor,
    const std::vector<int64_t>& im2col_op_params) {
    // get params
    const int64_t KH = im2col_op_params[0];
    const int64_t KW = im2col_op_params[1];
    const int64_t IW = im2col_op_params[2];
    const int64_t IC = im2col_op_params[3];
    const int64_t N = im2col_op_params[4];
    const int64_t OH = im2col_op_params[5];
    const int64_t OW = im2col_op_params[6];
    const int64_t s0 = im2col_op_params[7];
    const int64_t p0 = im2col_op_params[8];
    const int64_t d0 = im2col_op_params[9];
    const int64_t n_bytes_factor = im2col_op_params[10];

    // Permute: [N, IC * KH * KW, OW * OH] ->
    // [N, OW * OH * n_bytes_factor, IC * KH * KW]
    aclTensor* tmp_permute_tensor = nullptr;
    ggml_cann_pool_alloc tmp_permute_allocator(ctx.pool());
    tmp_permute_allocator.alloc(ggml_nbytes(dst) * n_bytes_factor);
    void* tmp_permute_buffer = tmp_permute_allocator.get();

    int64_t tmp_permute_ne[] = {IC * KH * KW, OW * OH * n_bytes_factor, N};
    size_t tmp_permute_nb[GGML_MAX_DIMS - 1];
    tmp_permute_nb[0] = ggml_type_size(dst->type);
    for (int i = 1; i < GGML_MAX_DIMS - 1; i++) {
        tmp_permute_nb[i] = tmp_permute_nb[i - 1] * tmp_permute_ne[i - 1];
    }

    tmp_permute_tensor = ggml_cann_create_tensor(
        tmp_permute_buffer, ggml_cann_type_mapping(dst->type),
        ggml_type_size(dst->type), tmp_permute_ne, tmp_permute_nb,
        GGML_MAX_DIMS - 1, ACL_FORMAT_ND);

    int64_t permute_dim[] = {0, 2, 1};
    if (src1->type != dst->type) {
        aclnn_permute(ctx, tmp_cast_tensor, tmp_permute_tensor, permute_dim, 3);
    } else {
        aclnn_permute(ctx, tmp_im2col_tensor, tmp_permute_tensor, permute_dim,
                      3);
    }

    // number of times the kernel moves in W dimension
    const int n_step_w = (IW + 2 * p0 - d0 * (KW - 1) - 1) / s0 + 1;
    size_t offset;
    void *cur_dst_buffer = dst->data, *cur_permute_buffer = tmp_permute_buffer;

    // memory copy with offset to restore 1D im2col from 2d
    if (IC > 1) {
        offset = IC * KH * KW * n_step_w * ggml_type_size(dst->type);
        size_t size_cpy = KH * KW * ggml_type_size(dst->type);

        for (int c = 0; c < IC; c++) {
            cur_permute_buffer = (char*)tmp_permute_buffer + offset +
                                 KH * KW * c * ggml_type_size(dst->type);
            cur_dst_buffer = (char*)dst->data +
                             c * KH * KW * n_step_w * ggml_type_size(dst->type);

            for (int i = 0; i < n_step_w; i++) {
                ACL_CHECK(aclrtMemcpyAsync(
                    cur_dst_buffer, size_cpy, cur_permute_buffer, size_cpy,
                    ACL_MEMCPY_DEVICE_TO_DEVICE, ctx.stream()));
                cur_dst_buffer =
                    (char*)cur_dst_buffer + KH * KW * ggml_type_size(dst->type);
                cur_permute_buffer = (char*)cur_permute_buffer +
                                     KH * KW * IC * ggml_type_size(dst->type);
            }
        }
    } else {
        offset = KH * KW * n_step_w *
                 ggml_type_size(dst->type);  // equal to ggml_nbytes(dst)
        ACL_CHECK(aclrtMemcpyAsync(dst->data, offset,
                                   (char*)tmp_permute_buffer + offset, offset,
                                   ACL_MEMCPY_DEVICE_TO_DEVICE, ctx.stream()));
    }

    // release
    ACL_CHECK(aclDestroyTensor(tmp_permute_tensor));
}

void ggml_cann_im2col(ggml_backend_cann_context& ctx, ggml_tensor* dst) {
    ggml_tensor* src0 = dst->src[0];  // kernel
    ggml_tensor* src1 = dst->src[1];  // input

    GGML_ASSERT(src0->type == GGML_TYPE_F16);
    GGML_ASSERT(src1->type == GGML_TYPE_F32);
    GGML_ASSERT(dst->type == GGML_TYPE_F16 || dst->type == GGML_TYPE_F32);

    GGML_TENSOR_BINARY_OP_LOCALS;

    // aclnnIm2col only works on 2D. set s1, p1, d1 to 1 to perform 2D
    // im2col and do post-processing to restore it to 1D.
    const bool is_2D = ((const int32_t*)(dst->op_params))[6] == 1;
    const int32_t s0 = ((const int32_t*)(dst->op_params))[0];
    const int32_t s1 = is_2D ? ((const int32_t*)(dst->op_params))[1] : 1;
    const int32_t p0 = ((const int32_t*)(dst->op_params))[2];
    const int32_t p1 = is_2D ? ((const int32_t*)(dst->op_params))[3] : 1;
    const int32_t d0 = ((const int32_t*)(dst->op_params))[4];
    const int32_t d1 = is_2D ? ((const int32_t*)(dst->op_params))[5] : 1;

    const int64_t N = ne13;
    const int64_t IC = ne12;
    const int64_t KH = ne01;
    const int64_t KW = ne00;
    const int64_t IW = ne10;

    const int64_t OH = is_2D ? ne2 : 1;
    const int64_t OW = ne1;

    GGML_ASSERT(nb00 == sizeof(ggml_fp16_t));
    GGML_ASSERT(nb10 == sizeof(float));

    // memory allocated increased to 3x when is_2D == false
    const int64_t n_bytes_factor = is_2D ? 1 : 3;

    // im2col: [N,C,H,W] -> [N, IC * KH * KW, OW * OH * n_bytes_factor]
    aclTensor* acl_src1 = ggml_cann_create_tensor(src1);
    int64_t tmp_im2col_ne[] = {OW * OH * n_bytes_factor, IC * KH * KW, N};
    size_t tmp_im2col_nb[GGML_MAX_DIMS - 1];

    tmp_im2col_nb[0] = ggml_type_size(src1->type);
    for (int i = 1; i < GGML_MAX_DIMS - 1; i++) {
        tmp_im2col_nb[i] = tmp_im2col_nb[i - 1] * tmp_im2col_ne[i - 1];
    }

    // Calculate im2col.
    // If dst is f16, tmp_buffer is f32, we need alloc src.typesize *
    // dst.elemcount.
    ggml_cann_pool_alloc im2col_allocator(
        ctx.pool(),
        ggml_nelements(dst) * ggml_element_size(src1) * n_bytes_factor);
    void* tmp_im2col_buffer = im2col_allocator.get();

    aclTensor* tmp_im2col_tensor = ggml_cann_create_tensor(
        tmp_im2col_buffer, ggml_cann_type_mapping(src1->type),
        ggml_type_size(src1->type), tmp_im2col_ne, tmp_im2col_nb,
        GGML_MAX_DIMS - 1, ACL_FORMAT_ND);

    std::vector<int64_t> kernel_dims = {KH, KW};
    std::vector<int64_t> dilation_size = {d1, d0};
    std::vector<int64_t> padding_dims = {p1, p0};
    std::vector<int64_t> stride_dims = {s1, s0};
    auto* kernel_size = aclCreateIntArray(kernel_dims.data(), 2);
    auto* dilations = aclCreateIntArray(dilation_size.data(), 2);
    auto* paddings = aclCreateIntArray(padding_dims.data(), 2);
    auto* strides = aclCreateIntArray(stride_dims.data(), 2);

    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    ACL_CHECK(aclnnIm2colGetWorkspaceSize(acl_src1, kernel_size, dilations,
                                          paddings, strides, tmp_im2col_tensor,
                                          &workspaceSize, &executor));

    ggml_cann_pool_alloc workspace_allocator(ctx.pool());
    if (workspaceSize > 0) {
        workspace_allocator.alloc(workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }

    ACL_CHECK(
        aclnnIm2col(workspaceAddr, workspaceSize, executor, ctx.stream()));

    // Cast if dst is f16.
    aclTensor* tmp_cast_tensor = nullptr;
    ggml_cann_pool_alloc tmp_cast_allocator(ctx.pool());
    void* tmp_cast_buffer = nullptr;
    if (src1->type != dst->type) {
        tmp_cast_allocator.alloc(ggml_nbytes(dst) * n_bytes_factor);
        tmp_cast_buffer = tmp_cast_allocator.get();
        size_t temp_cast_nb[GGML_MAX_DIMS - 1];
        temp_cast_nb[0] = ggml_type_size(dst->type);
        for (int i = 1; i < GGML_MAX_DIMS - 1; i++) {
            temp_cast_nb[i] = temp_cast_nb[i - 1] * tmp_im2col_ne[i - 1];
        }

        tmp_cast_tensor = ggml_cann_create_tensor(
            tmp_cast_buffer, ggml_cann_type_mapping(dst->type),
            ggml_type_size(dst->type), tmp_im2col_ne, temp_cast_nb,
            GGML_MAX_DIMS - 1, ACL_FORMAT_ND);
        aclnn_cast(ctx, tmp_im2col_tensor, tmp_cast_tensor,
                   ggml_cann_type_mapping(dst->type));
    }

    // post-processing
    if (is_2D) {
        ggml_cann_im2col_2d_post_process(ctx, dst, src1, tmp_cast_tensor,
                                         tmp_im2col_tensor);
    } else {
        std::vector<int64_t> im2col_op_params = {
            KH, KW, IW, IC, N, OH, OW, s0, p0, d0, n_bytes_factor};
        ggml_cann_im2col_1d_post_process(ctx, dst, src1, tmp_cast_tensor,
                                         tmp_im2col_tensor, im2col_op_params);
    }

    // release
    ACL_CHECK(aclDestroyTensor(acl_src1));
    ACL_CHECK(aclDestroyTensor(tmp_im2col_tensor));
    ACL_CHECK(aclDestroyTensor(tmp_cast_tensor));
    ACL_CHECK(aclDestroyIntArray(kernel_size));
    ACL_CHECK(aclDestroyIntArray(dilations));
    ACL_CHECK(aclDestroyIntArray(paddings));
    ACL_CHECK(aclDestroyIntArray(strides));
}

/**
 * @brief Applies element-wise exponential function to the elements of a tensor.
 *
 * This function computes the exponential of each element in the source tensor
 * `acl_src` and stores the result back into the same tensor.
 * The operation is defined as:
 * \f[
 *     \text {acl_src }_i=e^{acl\_src_i}
 * \f]
 *
 * @param ctx The context for the CANN backend operations.
 * @param acl_src The tensor on which the exponential function will be applied.
 */
static void aclnn_exp(ggml_backend_cann_context& ctx, aclTensor* acl_src) {
    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    ACL_CHECK(
        aclnnInplaceExpGetWorkspaceSize(acl_src, &workspaceSize, &executor));
    if (workspaceSize > 0) {
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }

    ACL_CHECK(
        aclnnInplaceExp(workspaceAddr, workspaceSize, executor, ctx.stream()));
}

/**
 * @brief Multiplies elements of a tensor by a scalar value, optionally
 * in-place.
 *
 * This function multiplies each element of the source tensor `acl_src` by the
 * scalar `scale` and stores the result in the destination tensor `acl_dst`. If
 * `inplace` is true, `acl_dst` will not be used and the operation is performed
 *  in-place on `acl_src`.
 * The operation is defined as:
 * \f[
 *     \text {acl_dst }_i=\text {acl_src }_i \times \text {scale}
 * \f]
 *
 * @param ctx The context for the CANN backend operations.
 * @param acl_src The source tensor whose elements will be multiplied.
 * @param scale The scalar value by which each element of `acl_src` will be
 * multiplied.
 * @param acl_dst The destination tensor where the result will be stored if
 * `inplace` is false.
 * @param inplace Flag indicating whether to perform the operation in-place on
 * `acl_src`.
 */
static void aclnn_muls(ggml_backend_cann_context& ctx, aclTensor* acl_src,
                       float scale, aclTensor* acl_dst, bool inplace) {
    aclScalar* acl_scale = aclCreateScalar(&scale, aclDataType::ACL_FLOAT);

    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    if (inplace) {
        ACL_CHECK(aclnnInplaceMulsGetWorkspaceSize(acl_src, acl_scale,
                                                   &workspaceSize, &executor));
        if (workspaceSize > 0) {
            ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
            workspaceAddr = workspace_allocator.get();
        }

        ACL_CHECK(aclnnInplaceMuls(workspaceAddr, workspaceSize, executor,
                                   ctx.stream()));
    } else {
        ACL_CHECK(aclnnMulsGetWorkspaceSize(acl_src, acl_scale, acl_dst,
                                            &workspaceSize, &executor));
        if (workspaceSize > 0) {
            ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
            workspaceAddr = workspace_allocator.get();
        }

        ACL_CHECK(
            aclnnMuls(workspaceAddr, workspaceSize, executor, ctx.stream()));
    }

    ACL_CHECK(aclDestroyScalar(acl_scale));
}

/**
 * @brief Performs an in-place element-wise multiplication of two tensors.
 *
 * This function performs an element-wise multiplication of the tensors
 * `acl_src` and `acl_other` and stores the result in `acl_src`.
 * The operation is defined as:
 * \f[
 *     \text {acl_src }_i=\text {acl_src }_i \times \text {acl_other }_i
 * \f]
 *
 * @param ctx The context for the CANN backend operations.
 * @param acl_src The source tensor where the multiplication result will be
 * stored.
 * @param acl_other The tensor whose elements will be multiplied with `acl_src`.
 */
static void aclnn_inplace_mul(ggml_backend_cann_context& ctx,
                              aclTensor* acl_src, aclTensor* acl_other) {
    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    ACL_CHECK(aclnnInplaceMulGetWorkspaceSize(acl_src, acl_other,
                                              &workspaceSize, &executor));
    if (workspaceSize > 0) {
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }

    ACL_CHECK(
        aclnnInplaceMul(workspaceAddr, workspaceSize, executor, ctx.stream()));
}

/**
 * @brief Performs element-wise multiplication of two tensors and stores the
 * result in a destination tensor.
 *
 * This function performs element-wise multiplication of the tensors `acl_src`
 * and `acl_other` and stores the result in the destination tensor `acl_dst`.
 * The operation is defined as:
 * \f[
 *     \text {acl_dst }_i=\text {acl_src }_i \times \text {acl_other }_i
 * \f]
 *
 * @param ctx The context for the CANN backend operations.
 * @param acl_src The first tensor for element-wise multiplication.
 * @param acl_other The second tensor for element-wise multiplication.
 * @param acl_dst The destination tensor where the result will be stored.
 */
static void aclnn_mul(ggml_backend_cann_context& ctx, aclTensor* acl_src,
                      aclTensor* acl_other, aclTensor* acl_dst) {
    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    ACL_CHECK(aclnnMulGetWorkspaceSize(acl_src, acl_other, acl_dst,
                                       &workspaceSize, &executor));
    if (workspaceSize > 0) {
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }

    ACL_CHECK(aclnnMul(workspaceAddr, workspaceSize, executor, ctx.stream()));
}

/**
 * @brief Applies element-wise cosine function to the elements of a tensor.
 *
 * This function computes the cosine of each element in the source tensor
 * `acl_src` and stores the result in the destination tensor `acl_dst`. The
 * operation is defined as: \f[ \text {acl_dst }_i=\cos \left(\text {acl_src
 * }_i\right) \f]
 *
 * @param ctx The context for the CANN backend operations.
 * @param acl_src The source tensor on which the cosine function will be
 * applied.
 * @param acl_dst The destination tensor where the cosine results will be
 * stored.
 */
static void aclnn_cos(ggml_backend_cann_context& ctx, aclTensor* acl_src,
                      aclTensor* acl_dst) {
    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    ACL_CHECK(
        aclnnCosGetWorkspaceSize(acl_src, acl_dst, &workspaceSize, &executor));
    if (workspaceSize > 0) {
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }

    ACL_CHECK(aclnnCos(workspaceAddr, workspaceSize, executor, ctx.stream()));
}

/**
 * @brief Applies element-wise sine function to the elements of a tensor.
 *
 * This function computes the sine of each element in the source tensor
 `acl_src`
 * and stores the result in the destination tensor `acl_dst`.
 * The operation is defined as:
 * \f[
 *     \text {acl_dst }_i=\sin \left(\text {acl_src }_i\right)
 * \f]

 * @param ctx The context for the CANN backend operations.
 * @param acl_src The source tensor on which the sine function will be applied.
 * @param acl_dst The destination tensor where the sine results will be stored.
 */
static void aclnn_sin(ggml_backend_cann_context& ctx, aclTensor* acl_src,
                      aclTensor* acl_dst) {
    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    ACL_CHECK(
        aclnnSinGetWorkspaceSize(acl_src, acl_dst, &workspaceSize, &executor));
    if (workspaceSize > 0) {
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }

    ACL_CHECK(aclnnSin(workspaceAddr, workspaceSize, executor, ctx.stream()));
}

void ggml_cann_timestep_embedding(ggml_backend_cann_context& ctx,
                                  ggml_tensor* dst) {
    const ggml_tensor* src = dst->src[0];

    GGML_ASSERT(src->type == GGML_TYPE_F32);
    GGML_ASSERT(dst->type == GGML_TYPE_F32);

    const int dim = dst->op_params[0];
    const int max_period = dst->op_params[1];
    int half = dim / 2;

    aclTensor* acl_src = ggml_cann_create_tensor(src);

    // arange: [0, ..., half)
    float start = 0;
    float stop = half;
    float step = 1;
    int64_t n_elements_arange = half;
    int64_t tmp_arange_ne[] = {half};
    size_t tmp_arange_nb[] = {sizeof(dst->type)};

    ggml_cann_pool_alloc arange_allocator(ctx.pool(), half * sizeof(dst->type));
    void* tmp_arange_buffer = arange_allocator.get();
    aclTensor* tmp_arange_tensor = ggml_cann_create_tensor(
        tmp_arange_buffer, ggml_cann_type_mapping(dst->type),
        ggml_type_size(dst->type), tmp_arange_ne, tmp_arange_nb,
        GGML_MAX_DIMS - 3, ACL_FORMAT_ND);

    aclnn_arange(ctx, tmp_arange_tensor, start, stop, step, n_elements_arange);

    // freq
    float freq_param = -logf(max_period) / half;
    bool inplace = true;
    aclnn_muls(ctx, tmp_arange_tensor, freq_param, nullptr, inplace);
    aclnn_exp(ctx, tmp_arange_tensor);

    // permute: src [0,1,2,3]->[0,1,3,2]
    int64_t tmp_permute_ne[] = {src->ne[1], src->ne[0], src->ne[2], src->ne[3]};
    size_t tmp_permute_nb[GGML_MAX_DIMS];
    tmp_permute_nb[0] = ggml_type_size(src->type);
    for (int i = 1; i < GGML_MAX_DIMS; i++) {
        tmp_permute_nb[i] = tmp_permute_nb[i - 1] * tmp_permute_ne[i - 1];
    }

    ggml_cann_pool_alloc permute_allocator(ctx.pool(), ggml_nbytes(src));
    void* tmp_permute_buffer = permute_allocator.get();
    aclTensor* tmp_permute_tenosr = ggml_cann_create_tensor(
        tmp_permute_buffer, ggml_cann_type_mapping(src->type),
        ggml_type_size(src->type), tmp_permute_ne, tmp_permute_nb,
        GGML_MAX_DIMS, ACL_FORMAT_ND);
    int64_t permute_dim[] = {0, 1, 3, 2};
    int64_t num_dims = 4;
    aclnn_permute(ctx, acl_src, tmp_permute_tenosr, permute_dim, num_dims);

    // timestep * freq
    int64_t tmp_mul_ne[] = {src->ne[1] * half, src->ne[0], src->ne[2],
                            src->ne[3]};
    size_t tmp_mul_nb[GGML_MAX_DIMS];
    tmp_mul_nb[0] = ggml_type_size(src->type);
    for (int i = 1; i < GGML_MAX_DIMS; i++) {
        tmp_mul_nb[i] = tmp_mul_nb[i - 1] * tmp_mul_ne[i - 1];
    }

    int mul_nelements =
        src->ne[1] * half * src->ne[0] * src->ne[2] * src->ne[3];

    ggml_cann_pool_alloc mul_allocator(
        ctx.pool(), mul_nelements * ggml_type_size(src->type));
    void* tmp_mul_buffer = mul_allocator.get();
    aclTensor* tmp_mul_tensor = ggml_cann_create_tensor(
        tmp_mul_buffer, ggml_cann_type_mapping(src->type),
        ggml_type_size(src->type), tmp_mul_ne, tmp_mul_nb, GGML_MAX_DIMS,
        ACL_FORMAT_ND);
    aclnn_mul(ctx, tmp_permute_tenosr, tmp_arange_tensor, tmp_mul_tensor);

    // cos
    ggml_cann_pool_alloc cos_allocator(
        ctx.pool(), mul_nelements * ggml_type_size(src->type));
    void* tmp_cos_buffer = cos_allocator.get();
    aclTensor* tmp_cos_tensor = ggml_cann_create_tensor(
        tmp_cos_buffer, ggml_cann_type_mapping(dst->type),
        ggml_type_size(dst->type), tmp_mul_ne, tmp_mul_nb, GGML_MAX_DIMS,
        ACL_FORMAT_ND);

    aclnn_cos(ctx, tmp_mul_tensor, tmp_cos_tensor);

    // sin
    ggml_cann_pool_alloc sin_allocator(
        ctx.pool(), mul_nelements * ggml_type_size(src->type));
    void* tmp_sin_buffer = sin_allocator.get();
    aclTensor* tmp_sin_tensor = ggml_cann_create_tensor(
        tmp_sin_buffer, ggml_cann_type_mapping(dst->type),
        ggml_type_size(dst->type), tmp_mul_ne, tmp_mul_nb, GGML_MAX_DIMS,
        ACL_FORMAT_ND);

    aclnn_sin(ctx, tmp_mul_tensor, tmp_sin_tensor);

    // concat
    int64_t concat_dim = 3;
    aclTensor* acl_dst = ggml_cann_create_tensor(dst);
    aclTensor* tensors[] = {tmp_cos_tensor, tmp_sin_tensor};
    aclTensorList* tensorList = aclCreateTensorList(tensors, 2);
    aclnn_concat(ctx, tensorList, acl_dst, concat_dim);

    // release
    // segmentation fault when delete both tensorList and his elements.
    ACL_CHECK(aclDestroyTensorList(tensorList));
    ACL_CHECK(aclDestroyTensor(acl_src));
    ACL_CHECK(aclDestroyTensor(tmp_arange_tensor));
    ACL_CHECK(aclDestroyTensor(tmp_permute_tenosr));
    ACL_CHECK(aclDestroyTensor(tmp_mul_tensor));
    ACL_CHECK(aclDestroyTensor(acl_dst));
}

/**
 * @brief Fills a tensor with a scalar value.
 *
 * This function fills the destination tensor `acl_dst` with the scalar value
 * `scalar`.
 *
 * @param ctx The context for the CANN backend operations.
 * @param scalar The scalar value used to fill the tensor.
 * @param acl_dst The destination tensor to be filled with the scalar value.
 */
static void aclnn_fill_scalar(ggml_backend_cann_context& ctx, float scalar,
                              aclTensor* acl_dst) {
    auto acl_scalar = aclCreateScalar(&scalar, aclDataType::ACL_FLOAT);

    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    ACL_CHECK(aclnnInplaceFillScalarGetWorkspaceSize(
        acl_dst, acl_scalar, &workspaceSize, &executor));
    if (workspaceSize > 0) {
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }

    ACL_CHECK(aclnnInplaceFillScalar(workspaceAddr, workspaceSize, executor,
                                     ctx.stream()));
    ACL_CHECK(aclDestroyScalar(acl_scalar));
}

/**
 * @brief Raises each element of a tensor to the power of the corresponding
 * element in another tensor.
 *
 * This function computes the element-wise power of the destination tensor
 * `acl_dst` raised to the power of the exponent tensor `acl_exp`.
 * The operation is defined as:
 * \f[
 *     \text {acl_dst }_i=acl\_dst_i^{\text {acl_exp }_i}
 * \f]
 *
 * @param ctx The context for the CANN backend operations.
 * @param acl_dst The destination tensor, which also serves as the base tensor.
 * @param acl_exp The exponent tensor, each element of which is used to raise
 * the corresponding element in the destination tensor.
 */
static void aclnn_pow_tensor_tensor(ggml_backend_cann_context& ctx,
                                    aclTensor* acl_dst, aclTensor* acl_exp) {
    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    ACL_CHECK(aclnnInplacePowTensorTensorGetWorkspaceSize(
        acl_dst, acl_exp, &workspaceSize, &executor));
    if (workspaceSize > 0) {
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }

    ACL_CHECK(aclnnInplacePowTensorTensor(workspaceAddr, workspaceSize,
                                          executor, ctx.stream()));
}

/**
 * @brief   Applies the Alibi (Attention with Linear Biases) mechanism to the
 * @details This function implements the Alibi mechanism, which introduces
 *          learnable biases into the attention scores to simulate relative
 *          position encoding without the need for explicit positional
 *          embeddings.
 *
 * @param ctx          The backend CANN context for executing operations.
 * @param acl_src      The source tensor representing the query or key.
 * @param acl_position The position tensor containing relative positions.
 * @param acl_dst      The destination tensor where the result will be stored.
 * @param n_head       The number of attention heads.
 * @param src_ne       The dimensions of the source tensor.
 * @param src_nb0      The byte size of the first dimension of the source
 tensor.
 * @param max_bias     The maximum bias value used in the Alibi mechanism.
 * @param dst          The destination tensor object for additional metadata.
 *
 * The function performs the following steps:
 * 1. Calculates the logarithm floor of the number of heads to determine the
      base for bias calculation.
 * 2. Initializes arrays with arithmetic sequences and fills them with bias
      values.
 * 3. Computes the bias tensor based on the calculated biases and arithmetic
      sequences.
 * 4. Reshapes the bias tensor to match the dimensions of the input tensors.
 * 5. Multiplies the position tensor by the bias tensor.
 * 6. Adds the result of the multiplication to the source tensor to produce the
      final output.
 */
static void aclnn_alibi(ggml_backend_cann_context& ctx, aclTensor* acl_src,
                        aclTensor* acl_position, aclTensor* acl_dst,
                        const int n_head, int64_t* src_ne, const size_t src_nb0,
                        float max_bias, ggml_tensor* dst) {
    const int64_t ne2_ne3 = src_ne[2] * src_ne[3];
    GGML_ASSERT(src_nb0 == sizeof(float));
    GGML_ASSERT(n_head == src_ne[2]);

    const int n_heads_log2_floor = 1u << (uint32_t)floor(log2(n_head));

    float m0 = powf(2.0f, -(max_bias) / n_heads_log2_floor);
    float m1 = powf(2.0f, -(max_bias / 2.0f) / n_heads_log2_floor);

    // init arange
    ggml_cann_pool_alloc arange_allocator(ctx.pool(),
                                          ne2_ne3 * ggml_type_size(dst->type));
    void* tmp_arange_buffer = arange_allocator.get();

    // arange1: [1, ..., n_heads_log2_floor+1)
    float start = 1;
    float stop = n_heads_log2_floor + 1;
    float step = 1;
    int64_t n_elements_arange = n_heads_log2_floor;

    int64_t tmp_arange1_ne[] = {n_heads_log2_floor};
    size_t tmp_arange1_nb[] = {sizeof(dst->type)};
    aclTensor* tmp_arange1_tensor = ggml_cann_create_tensor(
        tmp_arange_buffer, ggml_cann_type_mapping(dst->type),
        ggml_type_size(dst->type), tmp_arange1_ne, tmp_arange1_nb,
        GGML_MAX_DIMS - 3, ACL_FORMAT_ND);

    aclnn_arange(ctx, tmp_arange1_tensor, start, stop, step, n_elements_arange);

    aclTensor* tmp_arange2_tensor = nullptr;
    if (n_heads_log2_floor < ne2_ne3) {
        // arange2: [1, ..., 2 * (k - n_heads_log2_floor) + 1)
        start = 1;
        stop = 2 * (ne2_ne3 - n_heads_log2_floor) + 1;
        step = 2;
        n_elements_arange = ne2_ne3 - n_heads_log2_floor;
        int64_t tmp_arange2_ne[] = {ne2_ne3 - n_heads_log2_floor};
        size_t tmp_arange2_nb[] = {sizeof(dst->type)};

        aclTensor* tmp_arange2_tensor = ggml_cann_create_tensor(
            (char*)tmp_arange_buffer +
                n_heads_log2_floor * ggml_type_size(dst->type),
            ggml_cann_type_mapping(dst->type), ggml_type_size(dst->type),
            tmp_arange2_ne, tmp_arange2_nb, GGML_MAX_DIMS - 3, ACL_FORMAT_ND);
        aclnn_arange(ctx, tmp_arange2_tensor, start, stop, step,
                     n_elements_arange);
    }

    // init mk_base
    ggml_cann_pool_alloc mk_base_allocator(ctx.pool(),
                                           ne2_ne3 * ggml_type_size(dst->type));
    void* tmp_mk_base_buffer = mk_base_allocator.get();
    int64_t tmp_mk_base1_ne[] = {n_heads_log2_floor};
    size_t tmp_mk_base1_nb[] = {sizeof(dst->type)};
    aclTensor* tmp_mk_base1_tensor = ggml_cann_create_tensor(
        tmp_mk_base_buffer, ggml_cann_type_mapping(dst->type),
        ggml_type_size(dst->type), tmp_mk_base1_ne, tmp_mk_base1_nb,
        GGML_MAX_DIMS - 3, ACL_FORMAT_ND);

    aclnn_fill_scalar(ctx, m0, tmp_mk_base1_tensor);

    aclTensor* tmp_mk_base2_tensor = nullptr;
    if (n_heads_log2_floor < ne2_ne3) {
        int64_t tmp_mk_base2_ne[] = {ne2_ne3 - n_heads_log2_floor};
        size_t tmp_mk_base2_nb[] = {sizeof(dst->type)};
        aclTensor* tmp_mk_base2_tensor = ggml_cann_create_tensor(
            (char*)tmp_mk_base_buffer +
                n_heads_log2_floor * ggml_type_size(dst->type),
            ggml_cann_type_mapping(dst->type), ggml_type_size(dst->type),
            tmp_mk_base2_ne, tmp_mk_base2_nb, GGML_MAX_DIMS - 3, ACL_FORMAT_ND);
        aclnn_fill_scalar(ctx, m1, tmp_mk_base2_tensor);
    }

    // init mk
    int64_t tmp_mk_base_ne[] = {ne2_ne3};
    size_t tmp_mk_base_nb[] = {sizeof(dst->type)};
    aclTensor* tmp_mk_base_tensor = ggml_cann_create_tensor(
        tmp_mk_base_buffer, ggml_cann_type_mapping(dst->type),
        ggml_type_size(dst->type), tmp_mk_base_ne, tmp_mk_base_nb,
        GGML_MAX_DIMS - 3, ACL_FORMAT_ND);
    aclTensor* tmp_arange_tensor = ggml_cann_create_tensor(
        tmp_arange_buffer, ggml_cann_type_mapping(dst->type),
        ggml_type_size(dst->type), tmp_mk_base_ne, tmp_mk_base_nb,
        GGML_MAX_DIMS - 3, ACL_FORMAT_ND);
    aclnn_pow_tensor_tensor(ctx, tmp_mk_base_tensor, tmp_arange_tensor);

    // reshape mk
    int64_t tmp_mk_ne[] = {1, 1, src_ne[2], src_ne[3]};
    size_t tmp_mk_nb[GGML_MAX_DIMS];
    tmp_mk_nb[0] = ggml_type_size(dst->type);
    for (int i = 1; i < GGML_MAX_DIMS; i++) {
        tmp_mk_nb[i] = tmp_mk_nb[i - 1] * tmp_mk_ne[i - 1];
    }
    aclTensor* tmp_mk_tensor = ggml_cann_create_tensor(
        tmp_mk_base_buffer, ggml_cann_type_mapping(dst->type),
        ggml_type_size(dst->type), tmp_mk_ne, tmp_mk_nb, GGML_MAX_DIMS,
        ACL_FORMAT_ND);

    // acl_position * mk
    int64_t tmp_output_ne[] = {src_ne[0], src_ne[1], src_ne[2], src_ne[3]};
    size_t tmp_output_nb[GGML_MAX_DIMS];
    tmp_output_nb[0] = ggml_type_size(dst->type);
    for (int i = 1; i < GGML_MAX_DIMS; i++) {
        tmp_output_nb[i] = tmp_output_nb[i - 1] * tmp_output_ne[i - 1];
    }
    ggml_cann_pool_alloc output_allocator(ctx.pool(), ggml_nbytes(dst));
    void* tmp_output_buffer = output_allocator.get();
    aclTensor* tmp_output_tensor = ggml_cann_create_tensor(
        tmp_output_buffer, ggml_cann_type_mapping(dst->type),
        ggml_type_size(dst->type), tmp_output_ne, tmp_output_nb, GGML_MAX_DIMS,
        ACL_FORMAT_ND);
    aclnn_mul(ctx, acl_position, tmp_mk_tensor, tmp_output_tensor);

    // add
    aclnn_add(ctx, tmp_output_tensor, acl_src, acl_dst);

    ACL_CHECK(aclDestroyTensor(tmp_arange1_tensor));
    ACL_CHECK(aclDestroyTensor(tmp_arange2_tensor));
    ACL_CHECK(aclDestroyTensor(tmp_mk_base1_tensor));
    ACL_CHECK(aclDestroyTensor(tmp_mk_base2_tensor));
    ACL_CHECK(aclDestroyTensor(tmp_mk_base_tensor));
    ACL_CHECK(aclDestroyTensor(tmp_arange_tensor));
    ACL_CHECK(aclDestroyTensor(tmp_mk_tensor));
    ACL_CHECK(aclDestroyTensor(tmp_output_tensor));
}

void ggml_cann_cpy(ggml_backend_cann_context& ctx, ggml_tensor* dst) {
    ggml_cann_dup(ctx, dst);
}

/**
 * @brief Performs element-wise addition of two tensors in place.
 *
 * This function adds the source tensor `acl_src` to the destination tensor
 * `acl_dst` element-wise and stores the result in the destination tensor
 * `acl_dst`.
 *
 * @param ctx The context for the CANN backend operations.
 * @param acl_src The source tensor to be added.
 * @param acl_dst The destination tensor which will hold the result of the
 * addition.
 */
static void aclnn_inplace_add(ggml_backend_cann_context& ctx,
                              aclTensor* acl_src, aclTensor* acl_dst) {
    aclScalar* alpha = nullptr;
    float alphaValue = 1.0f;
    alpha = aclCreateScalar(&alphaValue, aclDataType::ACL_FLOAT);

    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    ACL_CHECK(aclnnInplaceAddGetWorkspaceSize(acl_dst, acl_src, alpha,
                                              &workspaceSize, &executor));
    if (workspaceSize > 0) {
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }

    ACL_CHECK(
        aclnnInplaceAdd(workspaceAddr, workspaceSize, executor, ctx.stream()));

    ACL_CHECK(aclDestroyScalar(alpha));
}

/**
 * @brief Applies the softmax function to a tensor along a specified dimension.
 *
 * This function computes the softmax of the source tensor `acl_src` along the
 * specified dimension `dim` and stores the result in the destination tensor
 * `acl_dst`.
 *
 * @param ctx The context for the CANN backend operations.
 * @param acl_src The source tensor on which the softmax function will be
 * applied.
 * @param dim The dimension along which the softmax function will be computed.
 * @param acl_dst The destination tensor where the softmax results will be
 * stored.
 */
static void aclnn_softmax(ggml_backend_cann_context& ctx, aclTensor* acl_src,
                          int64_t dim, aclTensor* acl_dst) {
    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    ACL_CHECK(aclnnSoftmaxGetWorkspaceSize(acl_src, dim, acl_dst,
                                           &workspaceSize, &executor));

    if (workspaceSize > 0) {
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }

    aclrtStream stream = ctx.stream();
    ACL_CHECK(aclnnSoftmax(workspaceAddr, workspaceSize, executor, stream));
}

void ggml_cann_softmax(ggml_backend_cann_context& ctx, ggml_tensor* dst) {
    ggml_tensor* src0 = dst->src[0];
    ggml_tensor* src1 = dst->src[1];  // mask

    aclTensor* acl_src0 = ggml_cann_create_tensor(src0);
    aclTensor* acl_dst = ggml_cann_create_tensor(dst);

    float scale = 1.0f;
    float max_bias = 0.0f;

    memcpy(&scale, (float*)dst->op_params + 0, sizeof(float));
    memcpy(&max_bias, (float*)dst->op_params + 1, sizeof(float));

    // input mul scale
    aclScalar* acl_scale = aclCreateScalar(&scale, aclDataType::ACL_FLOAT);

    size_t n_bytes = ggml_nbytes(src0);
    ggml_cann_pool_alloc mul_scale_allocator(ctx.pool(), n_bytes);
    void* input_mul_scale_buffer = mul_scale_allocator.get();
    aclTensor* acl_input_mul_scale_tensor = ggml_cann_create_tensor(
        input_mul_scale_buffer, ACL_FLOAT, ggml_type_size(src0->type), src0->ne,
        src0->nb, GGML_MAX_DIMS);

    bool inplace = false;
    aclnn_muls(ctx, acl_src0, scale, acl_input_mul_scale_tensor, inplace);

    // mask
    aclTensor* acl_src1_fp32_tensor = nullptr;
    aclTensor* tmp_mask_tensor = nullptr;
    ggml_cann_pool_alloc src1_fp32_allocator(ctx.pool());
    if (src1) {
        const bool use_f16 = src1->type == GGML_TYPE_F16;
        if (use_f16) {
            // cast to fp32
            size_t n_bytes = ggml_nelements(src1) * sizeof(float_t);
            size_t src1_fp32_nb[GGML_MAX_DIMS];
            src1_fp32_nb[0] = sizeof(float_t);
            for (int i = 1; i < GGML_MAX_DIMS; i++) {
                src1_fp32_nb[i] = src1_fp32_nb[i - 1] * src1->ne[i - 1];
            }
            src1_fp32_allocator.alloc(n_bytes);
            void* src1_fp32_buffer = src1_fp32_allocator.get();
            acl_src1_fp32_tensor = ggml_cann_create_tensor(
                src1_fp32_buffer, ACL_FLOAT, sizeof(float), src1->ne,
                src1_fp32_nb, GGML_MAX_DIMS);
            aclTensor* acl_src1 = ggml_cann_create_tensor(src1);
            aclnn_cast(ctx, acl_src1, acl_src1_fp32_tensor, ACL_FLOAT);

            ACL_CHECK(aclDestroyTensor(acl_src1));
        } else {
            acl_src1_fp32_tensor = ggml_cann_create_tensor(src1);
        }

        // broadcast the mask across rows, only use ne11 of ne01 in mask
        if (src1->ne[1] != src0->ne[1]) {
            // mask shape: [1,1,ne11,ne10]
            int64_t tmp_mask_ne[] = {src0->ne[0], src0->ne[1], 1, 1};
            size_t tmp_mask_nb[GGML_MAX_DIMS];
            tmp_mask_nb[0] = sizeof(float_t);
            for (int i = 1; i < GGML_MAX_DIMS; i++) {
                tmp_mask_nb[i] = tmp_mask_nb[i - 1] * tmp_mask_ne[i - 1];
            }
            tmp_mask_tensor = ggml_cann_create_tensor(
                src1->data, ACL_FLOAT, sizeof(float), tmp_mask_ne, tmp_mask_nb,
                GGML_MAX_DIMS, ACL_FORMAT_ND);
        }

        // alibi
        const int n_head = src0->ne[2];
        const size_t src_nb0 = src0->nb[0];

        n_bytes = ggml_nbytes(dst);
        ggml_cann_pool_alloc output_allocator(ctx.pool(), n_bytes);
        void* output_buffer = output_allocator.get();
        aclTensor* alibi_output_tensor = ggml_cann_create_tensor(
            output_buffer, ACL_FLOAT, ggml_type_size(dst->type), dst->ne,
            dst->nb, GGML_MAX_DIMS);
        if (max_bias <= 0.0f) {
            // slope = 1.0
            if (tmp_mask_tensor) {
                aclnn_add(ctx, tmp_mask_tensor, acl_input_mul_scale_tensor,
                          alibi_output_tensor);
            } else {
                aclnn_add(ctx, acl_src1_fp32_tensor, acl_input_mul_scale_tensor,
                          alibi_output_tensor);
            }
        } else {
            // slope != 1.0
            if (tmp_mask_tensor) {
                aclnn_alibi(ctx, acl_input_mul_scale_tensor, tmp_mask_tensor,
                            alibi_output_tensor, n_head, src0->ne, src_nb0,
                            max_bias, dst);
            } else {
                aclnn_alibi(ctx, acl_input_mul_scale_tensor,
                            acl_src1_fp32_tensor, alibi_output_tensor, n_head,
                            src0->ne, src_nb0, max_bias, dst);
            }
        }

        // softmax
        aclnn_softmax(ctx, alibi_output_tensor, 3, acl_dst);
        ACL_CHECK(aclDestroyTensor(alibi_output_tensor));
    } else {
        aclnn_softmax(ctx, acl_input_mul_scale_tensor, 3, acl_dst);
    }

    ACL_CHECK(aclDestroyTensor(acl_src0));
    ACL_CHECK(aclDestroyTensor(acl_src1_fp32_tensor));
    ACL_CHECK(aclDestroyTensor(acl_dst));
    ACL_CHECK(aclDestroyScalar(acl_scale));
    ACL_CHECK(aclDestroyTensor(acl_input_mul_scale_tensor));
    ACL_CHECK(aclDestroyTensor(tmp_mask_tensor));
}

void ggml_cann_get_rows(ggml_backend_cann_context& ctx, ggml_tensor* dst) {
    ggml_tensor* src0 = dst->src[0];
    ggml_tensor* src1 = dst->src[1];

    ggml_cann_pool_alloc src0_extra_allocator(ctx.pool(), sizeof(ggml_tensor));
    ggml_cann_pool_alloc src1_extra_allocator(ctx.pool(), sizeof(ggml_tensor));
    ggml_cann_pool_alloc dst_extra_allocator(ctx.pool(), sizeof(ggml_tensor));
    src0->extra = src0_extra_allocator.get();
    src1->extra = src1_extra_allocator.get();
    dst->extra = dst_extra_allocator.get();
    ACL_CHECK(aclrtMemcpyAsync(src0->extra, sizeof(ggml_tensor), src0,
                               sizeof(ggml_tensor), ACL_MEMCPY_HOST_TO_DEVICE,
                               ctx.stream()));
    ACL_CHECK(aclrtMemcpyAsync(src1->extra, sizeof(ggml_tensor), src1,
                               sizeof(ggml_tensor), ACL_MEMCPY_HOST_TO_DEVICE,
                               ctx.stream()));
    ACL_CHECK(aclrtMemcpyAsync(dst->extra, sizeof(ggml_tensor), dst,
                               sizeof(ggml_tensor), ACL_MEMCPY_HOST_TO_DEVICE,
                               ctx.stream()));

    switch (src0->type) {
        case GGML_TYPE_F32:
            aclrtlaunch_ascendc_get_row_f32(
                24, ctx.stream(), src0->data, src1->data, dst->data,
                ((ggml_tensor*)src0->extra)->ne,
                ((ggml_tensor*)src0->extra)->nb,
                ((ggml_tensor*)src1->extra)->ne,
                ((ggml_tensor*)src1->extra)->nb, ((ggml_tensor*)dst->extra)->ne,
                ((ggml_tensor*)dst->extra)->nb);
            break;
        case GGML_TYPE_F16:
            aclrtlaunch_ascendc_get_row_f16(
                24, ctx.stream(), src0->data, src1->data, dst->data,
                ((ggml_tensor*)src0->extra)->ne,
                ((ggml_tensor*)src0->extra)->nb,
                ((ggml_tensor*)src1->extra)->ne,
                ((ggml_tensor*)src1->extra)->nb, ((ggml_tensor*)dst->extra)->ne,
                ((ggml_tensor*)dst->extra)->nb);
            break;
        case GGML_TYPE_Q4_0:
            aclrtlaunch_ascendc_get_row_q4_0(
                24, ctx.stream(), src0->data, src1->data, dst->data,
                ((ggml_tensor*)src0->extra)->ne,
                ((ggml_tensor*)src1->extra)->ne,
                ((ggml_tensor*)src1->extra)->nb, ((ggml_tensor*)dst->extra)->ne,
                ((ggml_tensor*)dst->extra)->nb);
            break;
        case GGML_TYPE_Q8_0:
            aclrtlaunch_ascendc_get_row_q8_0(
                24, ctx.stream(), src0->data, src1->data, dst->data,
                ((ggml_tensor*)src0->extra)->ne,
                ((ggml_tensor*)src1->extra)->ne,
                ((ggml_tensor*)src1->extra)->nb, ((ggml_tensor*)dst->extra)->ne,
                ((ggml_tensor*)dst->extra)->nb);
            break;
        default:
            GGML_ABORT("fatal error");
            break;
    }
}

/**
 * @brief Repeats elements of a tensor along a specified dimension.
 *
 * This function repeats each element of the source tensor `acl_src` a specified
 * number of times (`repeats`) along the specified dimension `dim` and stores
 * the result in the destination tensor `acl_dst`.
 *
 * @param ctx The context for the CANN backend operations.
 * @param acl_src The source tensor whose elements will be repeated.
 * @param acl_dst The destination tensor where the repeated elements will be
 * stored.
 * @param dim The dimension along which the elements will be repeated.
 * @param repeats The number of times each element will be repeated.
 * @param output_size The size of the output tensor.
 */
static void aclnn_repeat_interleave(ggml_backend_cann_context& ctx,
                                    aclTensor* acl_src, aclTensor* acl_dst,
                                    int64_t dim, int64_t repeats,
                                    int64_t output_size) {
    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    ACL_CHECK(aclnnRepeatInterleaveIntWithDimGetWorkspaceSize(
        acl_src, repeats, dim, output_size, acl_dst, &workspaceSize,
        &executor));
    if (workspaceSize > 0) {
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }

    ACL_CHECK(aclnnRepeatInterleaveIntWithDim(workspaceAddr, workspaceSize,
                                              executor, ctx.stream()));
}

/**
 * @brief Performs matrix multiplication of two tensors.
 *
 * This function computes the matrix multiplication of the input tensor
 * `acl_input` and the weight tensor `acl_weight`, and stores the result in the
 * destination tensor `acl_dst`.
 * The operation is defined as:
 * \f[
 *     \text {acl_dst}=\text {acl_input@acl_weight}
 * \f]
 *
 * @param ctx The context for the CANN backend operations.
 * @param acl_input The input tensor for the matrix multiplication.
 * @param acl_weight The weight tensor for the matrix multiplication.
 * @param acl_dst The destination tensor where the result of the matrix
 * multiplication will be stored.
 */
static void aclnn_mat_mul(ggml_backend_cann_context& ctx, aclTensor* acl_input,
                          aclTensor* acl_weight, aclTensor* acl_dst) {
    int8_t cube_math_type = 1;  // ALLOW_FP32_DOWN_PRECISION, when input is
                                // fp32, atlas a2 will transpose it to HFLOAT32.

    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    ACL_CHECK(aclnnMatmulGetWorkspaceSize(acl_input, acl_weight, acl_dst,
                                          cube_math_type, &workspaceSize,
                                          &executor));

    if (workspaceSize > 0) {
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }

    ACL_CHECK(
        aclnnMatmul(workspaceAddr, workspaceSize, executor, ctx.stream()));
}

/**
 * @brief Performs matrix multiplication with floating-point precision on
 * tensors using the CANN backend.
 *
 * This function performs matrix multiplication of the input tensor and the
 * weight tensor, handling broadcasting and transposing as needed, and stores
 * the result in the destination tensor `dst`.
 *
 * @param ctx The context for the CANN backend operations.
 * @param dst The destination tensor where the result of the matrix
 * multiplication will be stored.
 */
static void ggml_cann_mat_mul_fp(ggml_backend_cann_context& ctx,
                                 ggml_tensor* dst) {
    ggml_tensor* weight = dst->src[0];  // weight
    ggml_tensor* input = dst->src[1];   // input

    // when weight ne2 or ne3 is 1, aclnnMatmulGetWorkspaceSize will auto
    // broadcast, when weight ne2 or ne3 is not 1, weight need repeat.
    BCAST_MUL_MAT_SHAPE(input, weight, dst);

    // transpose weight: [1,2,3,4] -> [1,2,4,3]
    int64_t transpose_ne[] = {bcast_weight_ne[1], bcast_weight_ne[0],
                              bcast_weight_ne[2], bcast_weight_ne[3],
                              bcast_weight_ne[4], bcast_weight_ne[5]};
    size_t transpose_nb[] = {bcast_weight_nb[1], bcast_weight_nb[0],
                             bcast_weight_nb[2], bcast_weight_nb[3],
                             bcast_weight_nb[4], bcast_weight_nb[5]};

    aclTensor* acl_weight_tensor =
        ggml_cann_create_tensor(weight, transpose_ne, transpose_nb, bcast_dims);
    aclTensor* acl_input_tensor =
        ggml_cann_create_tensor(input, BCAST_MUL_MAT_PARAM(input));
    aclTensor* acl_dst = ggml_cann_create_tensor(dst, BCAST_MUL_MAT_PARAM(dst));
    aclnn_mat_mul(ctx, acl_input_tensor, acl_weight_tensor, acl_dst);

    ACL_CHECK(aclDestroyTensor(acl_weight_tensor));
    ACL_CHECK(aclDestroyTensor(acl_input_tensor));
    ACL_CHECK(aclDestroyTensor(acl_dst));
}

/**
 * @brief Performs matrix multiplication with quantized weights and
 * floating-point inputs using the CANN backend.
 *
 * This function performs matrix multiplication of the input tensor `src1` and
 * the weight tensor `src0`, handling broadcasting, transposing, and
 * quantization as needed, and stores the result in the destination tensor
 * `dst`.
 *
 * @param ctx The context for the CANN backend operations.
 * @param dst The destination tensor where the result of the matrix
 * multiplication will be stored.
 */
static void ggml_cann_mul_mat_quant(ggml_backend_cann_context& ctx,
                                   ggml_tensor* dst,
                                   const enum ggml_type type) {
    ggml_tensor* src0 = dst->src[0];  // weight
    ggml_tensor* src1 = dst->src[1];  // input

    // The shape of the weight is NCHW. Matrix multiplication uses HW dims. HC
    // is regarded as batch. weight need transpose.
    int64_t weight_ne[] = {src0->ne[1], src0->ne[0]};
    float weight_elem_size;
    if (type == GGML_TYPE_Q4_0) {
        weight_elem_size = float(sizeof(uint8_t)) / 2;
    }
    else if (type == GGML_TYPE_Q8_0) {
        weight_elem_size = float(sizeof(uint8_t));
    }
    else {
        GGML_ABORT("Only support Q4_0 and Q8_0 MUL_MAT");
    }
    float weight_nb[] = {weight_elem_size * src0->ne[0], weight_elem_size};

    // size of one matrix is element_size * height * width.
    size_t weight_stride = weight_elem_size * src0->ne[0] * src0->ne[1];
    size_t weight_size = weight_stride * src0->ne[2] * src0->ne[3];

    // scale stored at the end of weight. Also need transpose.
    GGML_ASSERT(QK4_0 == QK8_0);
    int64_t scale_ne[] = {src0->ne[1], src0->ne[0] / QK8_0};
    size_t scale_elem_size = sizeof(uint16_t);
    size_t scale_nb[] = {src0->ne[0] / QK8_0 * scale_elem_size,
                         scale_elem_size};
    size_t scale_stride = scale_elem_size * src0->ne[0] * src0->ne[1] / QK8_0;
    char* scale_offset = (char*)src0->data + weight_size;

    // input
    void* input_buffer;
    size_t input_elem_size = sizeof(uint16_t);
    int64_t input_ne[] = {src1->ne[0], src1->ne[1]};
    size_t input_nb[] = {input_elem_size, input_elem_size * src1->ne[0]};
    size_t input_stride = input_elem_size * src1->ne[0] * src1->ne[1];

    ggml_cann_pool_alloc input_alloctor(ctx.pool());
    if (src1->type != GGML_TYPE_F16) {
        aclTensor* acl_src1_tensor = ggml_cann_create_tensor(src1);
        input_alloctor.alloc(ggml_nelements(src1) * input_elem_size);
        input_buffer = input_alloctor.get();

        int64_t* input_cast_ne = src1->ne;
        size_t input_cast_nb[GGML_MAX_DIMS];
        input_cast_nb[0] = sizeof(uint16_t);
        for (int i = 1; i < GGML_MAX_DIMS; i++) {
            input_cast_nb[i] = input_cast_nb[i - 1] * input_cast_ne[i - 1];
        }

        aclTensor* acl_input_tensor = ggml_cann_create_tensor(
            input_buffer, ACL_FLOAT16, input_elem_size, input_cast_ne,
            input_cast_nb, GGML_MAX_DIMS);
        aclnn_cast(ctx, acl_src1_tensor, acl_input_tensor, ACL_FLOAT16);
        ACL_CHECK(aclDestroyTensor(acl_input_tensor));
        ACL_CHECK(aclDestroyTensor(acl_src1_tensor));
    } else {
        input_buffer = src1->data;
    }

    // output
    size_t output_elem_size = sizeof(uint16_t);
    int64_t output_ne[] = {dst->ne[0], dst->ne[1]};
    size_t output_nb[] = {output_elem_size, output_elem_size * dst->ne[0]};
    ggml_cann_pool_alloc output_alloctor(
        ctx.pool(), ggml_nelements(dst) * output_elem_size);
    void* output_buffer = output_alloctor.get();
    size_t output_stride = output_elem_size * dst->ne[0] * dst->ne[1];

    // aclnn
    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    for (int64_t n1 = 0; n1 < src1->ne[3]; n1++) {
        for (int64_t c1 = 0; c1 < src1->ne[2]; c1++) {
            int64_t n0 = n1 / (src1->ne[3] / src0->ne[3]);
            int64_t c0 = c1 / (src1->ne[2] / src0->ne[2]);

            int64_t batch1 = n1 * src1->ne[2] + c1;
            int64_t batch0 = n0 * src0->ne[2] + c0;

            aclTensor* acl_input_tensor = ggml_cann_create_tensor(
                (char*)input_buffer + batch1 * input_stride, ACL_FLOAT16,
                input_elem_size, input_ne, input_nb, 2);
            aclTensor* acl_weight_tensor = ggml_cann_create_tensor(
                (char*)src0->data + batch0 * weight_stride,
                ggml_cann_type_mapping(type), weight_elem_size, weight_ne,
                weight_nb, 2);
            aclTensor* acl_scale_tensor = ggml_cann_create_tensor(
                scale_offset + batch0 * scale_stride, ACL_FLOAT16,
                scale_elem_size, scale_ne, scale_nb, 2);
            aclTensor* acl_output_tensor = ggml_cann_create_tensor(
                (char*)output_buffer + batch1 * output_stride, ACL_FLOAT16,
                output_elem_size, output_ne, output_nb, 2);

            ACL_CHECK(aclnnWeightQuantBatchMatmulV2GetWorkspaceSize(
                acl_input_tensor, acl_weight_tensor, acl_scale_tensor, nullptr,
                nullptr, nullptr, nullptr, QK8_0, acl_output_tensor,
                &workspaceSize, &executor));

            if (workspaceSize > 0 && workspaceAddr == nullptr) {
                ggml_cann_pool_alloc workspace_allocator(ctx.pool(),
                                                         workspaceSize);
                workspaceAddr = workspace_allocator.get();
            }

            ACL_CHECK(aclnnWeightQuantBatchMatmulV2(
                workspaceAddr, workspaceSize, executor, ctx.stream()));

            ACL_CHECK(aclDestroyTensor(acl_input_tensor));
            ACL_CHECK(aclDestroyTensor(acl_weight_tensor));
            ACL_CHECK(aclDestroyTensor(acl_scale_tensor));
            ACL_CHECK(aclDestroyTensor(acl_output_tensor));
        }
    }

    // cast out
    int64_t* output_cast_ne = dst->ne;
    size_t output_cast_nb[GGML_MAX_DIMS];
    output_cast_nb[0] = sizeof(uint16_t);
    for (int i = 1; i < GGML_MAX_DIMS; i++) {
        output_cast_nb[i] = output_cast_nb[i - 1] * output_cast_ne[i - 1];
    }

    aclTensor* acl_output_tensor =
        ggml_cann_create_tensor(output_buffer, ACL_FLOAT16, output_elem_size,
                                output_cast_ne, output_cast_nb, GGML_MAX_DIMS);
    aclTensor* acl_dst_tensor = ggml_cann_create_tensor(dst);
    aclnn_cast(ctx, acl_output_tensor, acl_dst_tensor, ACL_FLOAT);

    ACL_CHECK(aclDestroyTensor(acl_output_tensor));
    ACL_CHECK(aclDestroyTensor(acl_dst_tensor));
}

void ggml_cann_mul_mat(ggml_backend_cann_context& ctx, ggml_tensor* dst) {
    const enum ggml_type type = dst->src[0]->type;
    switch (type) {
        case GGML_TYPE_F32:
        case GGML_TYPE_F16:
            ggml_cann_mat_mul_fp(ctx, dst);
            break;
        case GGML_TYPE_Q4_0:
        case GGML_TYPE_Q8_0:
            ggml_cann_mul_mat_quant(ctx, dst, type);
            break;
        default:
            GGML_ABORT("fatal error");
            break;
    }
}

/**
 * @brief Rolls the elements of a tensor along a specified dimension.
 *
 * This function rolls the elements of the source tensor `acl_src` by the
 * specified shifts `shifts` along the specified dimensions `dims`, and stores
 * the result in the destination tensor `acl_dst`.
 *
 * @param ctx The context for the CANN backend operations.
 * @param acl_src The source tensor whose elements will be rolled.
 * @param acl_dst The destination tensor where the rolled elements will be
 * stored.
 * @param shifts An array specifying the number of positions by which elements
 * are shifted.
 * @param dims An array specifying the dimensions along which elements are
 * shifted.
 */
static void aclnn_roll(ggml_backend_cann_context& ctx, aclTensor* acl_src,
                       aclTensor* acl_dst, int64_t* shifts, int64_t* dims) {
    aclIntArray* acl_shifts = aclCreateIntArray(shifts, 1);
    aclIntArray* acl_dims = aclCreateIntArray(dims, 1);

    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    ACL_CHECK(aclnnRollGetWorkspaceSize(acl_src, acl_shifts, acl_dims, acl_dst,
                                        &workspaceSize, &executor));
    if (workspaceSize > 0) {
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }

    ACL_CHECK(aclnnRoll(workspaceAddr, workspaceSize, executor, ctx.stream()));

    ACL_CHECK(aclDestroyIntArray(acl_shifts));
    ACL_CHECK(aclDestroyIntArray(acl_dims));
}

/**
 * @brief Fills specified positions of a tensor with a scalar value.
 *
 * This function fills the positions in the source tensor `acl_src` specified by
 * `index` along the dimension `dim` with the scalar value `value`.
 *
 * @param ctx The context for the CANN backend operations.
 * @param acl_src The source tensor where the positions will be filled.
 * @param dim The dimension along which the positions are specified.
 * @param index An array specifying the positions to be filled.
 * @param index_num The number of positions specified in the index array.
 * @param value The scalar value used to fill the specified positions.
 */
static void aclnn_index_fill_tensor(ggml_backend_cann_context& ctx,
                                    aclTensor* acl_src, int64_t dim,
                                    int64_t* index, int64_t index_num,
                                    float value) {
    aclIntArray* acl_index = aclCreateIntArray(index, index_num);
    aclScalar* acl_value = aclCreateScalar(&value, aclDataType::ACL_FLOAT);

    uint64_t workspaceSize = 0;
    aclOpExecutor* executor;
    void* workspaceAddr = nullptr;

    ACL_CHECK(aclnnInplaceIndexFillTensorGetWorkspaceSize(
        acl_src, dim, acl_index, acl_value, &workspaceSize, &executor));
    if (workspaceSize > 0) {
        ggml_cann_pool_alloc workspace_allocator(ctx.pool(), workspaceSize);
        workspaceAddr = workspace_allocator.get();
    }

    ACL_CHECK(aclnnInplaceIndexFillTensor(workspaceAddr, workspaceSize,
                                          executor, ctx.stream()));

    ACL_CHECK(aclDestroyIntArray(acl_index));
    ACL_CHECK(aclDestroyScalar(acl_value));
}

static void aclnn_cache_init(ggml_backend_cann_context& ctx, ggml_tensor* dst,
                             aclTensor* acl_cos_repeat_tensor,
                             aclTensor* acl_sin_repeat_tensor,
                             float theta_scale, bool is_neox) {
    // int sin/cos cache, cache has different repeat method depond on
    // @param.is_neox

    ggml_tensor* src0 = dst->src[0];  // input
    ggml_tensor* src1 = dst->src[1];  // position

    // arange, [0,1,...,ne0/2]
    int64_t arange_length = src0->ne[0] / 2;
    ggml_cann_pool_alloc arange_allocator(ctx.pool(),
                                          arange_length * sizeof(float_t));
    void* arange_buffer = arange_allocator.get();
    int64_t arange_ne[] = {arange_length, 1, 1, 1};
    size_t arange_nb[] = {sizeof(float_t), sizeof(float_t), sizeof(float_t),
                          arange_length * sizeof(float_t)};

    aclTensor* acl_arange_tensor =
        ggml_cann_create_tensor(arange_buffer, ACL_FLOAT, sizeof(float_t),
                                arange_ne, arange_nb, GGML_MAX_DIMS);
    float start = 0;
    float step = 1;
    float stop = src0->ne[0] / 2;
    float n_elements = src0->ne[0] / 2;
    aclnn_arange(ctx, acl_arange_tensor, start, stop, step, n_elements);

    // power
    // aclnnPowScalarTensor(): @param self is tensor which should be scalar, so
    // use aclnn_pow_tensor_tensor() until fixed. aclScalar* acl_theta_scale =
    // aclCreateScalar(&theta_scale, aclDataType::ACL_FLOAT);
    // aclnn_power_scalar_tensor(ctx, acl_theta_scale, acl_arange_tensor,
    // acl_power_tensor);
    ggml_cann_pool_alloc theta_scale_allocator(ctx.pool(),
                                               arange_length * sizeof(float_t));
    void* theta_scale_buffer = theta_scale_allocator.get();
    aclTensor* acl_theta_scale_tensor = aclnn_ones(
        ctx, theta_scale_buffer, arange_length * sizeof(float_t), arange_ne,
        GGML_MAX_DIMS, ACL_FLOAT, sizeof(float_t), theta_scale);
    aclnn_pow_tensor_tensor(ctx, acl_theta_scale_tensor, acl_arange_tensor);

    // position
    GGML_ASSERT(src1->type == GGML_TYPE_I32);
    int64_t position_length = src1->ne[0];
    int64_t position_ne[] = {1, position_length, 1, 1};
    size_t position_nb[] = {sizeof(int32_t), sizeof(int32_t),
                            sizeof(int32_t) * position_length,
                            sizeof(int32_t) * position_length};
    aclTensor* acl_position_tensor = ggml_cann_create_tensor(
        src1->data, ggml_cann_type_mapping(src1->type),
        ggml_type_size(src1->type), position_ne, position_nb, GGML_MAX_DIMS);

    // power * position
    int64_t theta_length = arange_length * position_length;
    ggml_cann_pool_alloc theta_allocator(ctx.pool(),
                                         theta_length * sizeof(float_t));
    void* theta_buffer = theta_allocator.get();
    int64_t theta_ne[] = {arange_length, position_length, 1, 1};
    size_t theta_nb[GGML_MAX_DIMS];
    theta_nb[0] = sizeof(float_t);
    for (int i = 1; i < GGML_MAX_DIMS; i++) {
        theta_nb[i] = theta_nb[i - 1] * theta_ne[i - 1];
    }
    aclTensor* acl_theta_tensor =
        ggml_cann_create_tensor(theta_buffer, ACL_FLOAT, sizeof(float_t),
                                theta_ne, theta_nb, GGML_MAX_DIMS);
    aclnn_mul(ctx, acl_position_tensor, acl_theta_scale_tensor,
              acl_theta_tensor);

    // permute: [0,1,2,3]->[0,2,1,3]
    int64_t permute_ne[] = {arange_length, 1, position_length, 1};
    size_t permute_nb[GGML_MAX_DIMS];
    permute_nb[0] = sizeof(float_t);
    for (int i = 1; i < GGML_MAX_DIMS; i++) {
        permute_nb[i] = permute_nb[i - 1] * permute_ne[i - 1];
    }
    ggml_cann_pool_alloc permute_allocator(ctx.pool(),
                                           theta_length * sizeof(float_t));
    void* permute_buffer = permute_allocator.get();
    aclTensor* acl_permute_tensor = ggml_cann_create_tensor(
        permute_buffer, ACL_FLOAT, sizeof(float_t), permute_ne, permute_nb,
        GGML_MAX_DIMS, ACL_FORMAT_ND);
    int64_t permute_dim[] = {0, 2, 1, 3};
    int64_t num_dims = 4;
    aclnn_permute(ctx, acl_theta_tensor, acl_permute_tensor, permute_dim,
                  num_dims);

    // sin/cos
    ggml_cann_pool_alloc sin_allocator(ctx.pool(),
                                       theta_length * sizeof(float_t));
    void* sin_buffer = sin_allocator.get();
    aclTensor* acl_sin_tensor = ggml_cann_create_tensor(
        sin_buffer, ACL_FLOAT, sizeof(float_t), permute_ne, permute_nb,
        GGML_MAX_DIMS, ACL_FORMAT_ND);
    aclnn_sin(ctx, acl_permute_tensor, acl_sin_tensor);

    ggml_cann_pool_alloc cos_allocator(ctx.pool(),
                                       theta_length * sizeof(float_t));
    void* cos_buffer = cos_allocator.get();
    aclTensor* acl_cos_tensor = ggml_cann_create_tensor(
        cos_buffer, ACL_FLOAT, sizeof(float_t), permute_ne, permute_nb,
        GGML_MAX_DIMS, ACL_FORMAT_ND);
    aclnn_cos(ctx, acl_permute_tensor, acl_cos_tensor);

    // repeat
    if (is_neox) {
        int64_t repeatsArray[] = {1, 1, 1, 2};
        aclnn_repeat(ctx, acl_sin_tensor, acl_sin_repeat_tensor, repeatsArray);
        aclnn_repeat(ctx, acl_cos_tensor, acl_cos_repeat_tensor, repeatsArray);
    } else {
        int64_t num_repeats = 2;
        int64_t dim = 3;
        int64_t output_size = arange_length * num_repeats;
        aclnn_repeat_interleave(ctx, acl_sin_tensor, acl_sin_repeat_tensor, dim,
                                num_repeats, output_size);
        aclnn_repeat_interleave(ctx, acl_cos_tensor, acl_cos_repeat_tensor, dim,
                                num_repeats, output_size);
    }

    // release
    ACL_CHECK(aclDestroyTensor(acl_arange_tensor));
    ACL_CHECK(aclDestroyTensor(acl_theta_scale_tensor));
    ACL_CHECK(aclDestroyTensor(acl_position_tensor));
    ACL_CHECK(aclDestroyTensor(acl_theta_tensor));
    ACL_CHECK(aclDestroyTensor(acl_permute_tensor));
    ACL_CHECK(aclDestroyTensor(acl_sin_tensor));
    ACL_CHECK(aclDestroyTensor(acl_cos_tensor));
}

void ggml_cann_rope(ggml_backend_cann_context& ctx, ggml_tensor* dst) {
    // TODO: use ascendc
    // Only test with LLAMA model.
    ggml_tensor* src0 = dst->src[0];  // input
    ggml_tensor* src2 = dst->src[2];  // freq_factors

    // TODO: with freq_factors
    GGML_ASSERT(src2 == NULL);

    // param
    float freq_base, freq_scale, ext_factor, attn_factor, beta_fast, beta_slow;
    // const int n_past     = ((int32_t *) dst->op_params)[0];
    const int n_dims = ((int32_t*)dst->op_params)[1];
    const int mode = ((int32_t*)dst->op_params)[2];
    // const int n_ctx      = ((int32_t *) dst->op_params)[3];
    const int n_ctx_orig = ((int32_t*)dst->op_params)[4];

    GGML_TENSOR_UNARY_OP_LOCALS

    memcpy(&freq_base, (int32_t*)dst->op_params + 5, sizeof(float));
    memcpy(&freq_scale, (int32_t*)dst->op_params + 6, sizeof(float));
    memcpy(&ext_factor, (int32_t*)dst->op_params + 7, sizeof(float));
    memcpy(&attn_factor, (int32_t*)dst->op_params + 8, sizeof(float));
    memcpy(&beta_fast, (int32_t*)dst->op_params + 9, sizeof(float));
    memcpy(&beta_slow, (int32_t*)dst->op_params + 10, sizeof(float));

    GGML_ASSERT(n_dims <= ne0);
    GGML_ASSERT(n_dims % 2 == 0);

    // TODO: ext_factor != 0
    GGML_ASSERT(ext_factor == 0);
    // TODO: freq_scale != 1
    GGML_ASSERT(freq_scale == 1);

    const float theta_scale = powf(freq_base, -2.0f / n_dims);

    float corr_dims[2];
    ggml_rope_yarn_corr_dims(n_dims, n_ctx_orig, freq_base, beta_fast,
                             beta_slow, corr_dims);

    const bool is_neox = mode & GGML_ROPE_TYPE_NEOX;

    // init cos/sin cache
    ggml_cann_pool_alloc sin_allocator(
        ctx.pool(), src0->ne[0] * src0->ne[2] * sizeof(float_t));
    ggml_cann_pool_alloc cos_allocator(
        ctx.pool(), src0->ne[0] * src0->ne[2] * sizeof(float_t));
    void* sin_buffer = sin_allocator.get();
    void* cos_buffer = cos_allocator.get();

    int64_t sin_reshape_ne[4] = {src0->ne[0], 1, src0->ne[2], 1};
    size_t sin_reshape_nb[GGML_MAX_DIMS];
    sin_reshape_nb[0] = sizeof(float_t);
    for (int i = 1; i < GGML_MAX_DIMS; i++) {
        sin_reshape_nb[i] = sin_reshape_nb[i - 1] * sin_reshape_ne[i - 1];
    }
    aclTensor* acl_sin_reshape_tensor =
        ggml_cann_create_tensor(sin_buffer, ACL_FLOAT, sizeof(float_t),
                                sin_reshape_ne, sin_reshape_nb, GGML_MAX_DIMS);
    aclTensor* acl_cos_reshape_tensor =
        ggml_cann_create_tensor(cos_buffer, ACL_FLOAT, sizeof(float_t),
                                sin_reshape_ne, sin_reshape_nb, GGML_MAX_DIMS);
    aclnn_cache_init(ctx, dst, acl_cos_reshape_tensor, acl_sin_reshape_tensor,
                     theta_scale, is_neox);

    // roll input
    void* input_roll_buffer;
    aclTensor* acl_minus_one_tensor;
    void* minus_one_scale_buffer = nullptr;
    ggml_cann_pool_alloc roll_allocator(ctx.pool(), ggml_nbytes(src0));
    ggml_cann_pool_alloc minus_one_scale_allocator(
        ctx.pool(), sizeof(float_t) * src0->ne[0]);
    if (!is_neox) {
        // roll input: [q0,q1,q2,q3,...] -> [q1,q0,q3,q2,...]
        input_roll_buffer = roll_allocator.get();
        int64_t input_roll_ne[4] = {2, src0->ne[1] * (src0->ne[0] / 2),
                                    src0->ne[2], src0->ne[3]};
        size_t input_roll_nb[GGML_MAX_DIMS];
        input_roll_nb[0] = ggml_type_size(src0->type);
        for (int i = 1; i < GGML_MAX_DIMS; i++) {
            input_roll_nb[i] = input_roll_nb[i - 1] * input_roll_ne[i - 1];
        }
        aclTensor* acl_input_roll_tensor = ggml_cann_create_tensor(
            input_roll_buffer, ggml_cann_type_mapping(src0->type),
            ggml_type_size(src0->type), input_roll_ne, input_roll_nb,
            GGML_MAX_DIMS);
        aclTensor* acl_input_tensor = ggml_cann_create_tensor(
            src0->data, ggml_cann_type_mapping(src0->type),
            ggml_type_size(src0->type), input_roll_ne, input_roll_nb,
            GGML_MAX_DIMS);

        int64_t shifts[] = {1};
        int64_t dims[] = {3};
        aclnn_roll(ctx, acl_input_tensor, acl_input_roll_tensor, shifts, dims);
        ACL_CHECK(aclDestroyTensor(acl_input_roll_tensor));
        ACL_CHECK(aclDestroyTensor(acl_input_tensor));

        // init [-1, 1, -1, 1, ...]
        minus_one_scale_buffer = minus_one_scale_allocator.get();

        int64_t minus_one_ne[4] = {src0->ne[0], 1, 1, 1};
        size_t minus_one_nb[GGML_MAX_DIMS];
        minus_one_nb[0] = sizeof(float_t);
        for (int i = 1; i < GGML_MAX_DIMS; i++) {
            minus_one_nb[i] = minus_one_nb[i - 1] * minus_one_ne[i - 1];
        }
        acl_minus_one_tensor = aclnn_ones(
            ctx, minus_one_scale_buffer, sizeof(float_t) * src0->ne[0],
            minus_one_ne, GGML_MAX_DIMS, ACL_FLOAT, sizeof(float_t), 1);
        int64_t dim = 3;
        int64_t* index = new int64_t[src0->ne[0]];
        for (int i = 0; i < src0->ne[0]; i++) {
            index[i] = i / 2 * 2;
        }
        int64_t index_num = src0->ne[0];
        float value = -1;
        aclnn_index_fill_tensor(ctx, acl_minus_one_tensor, dim, index,
                                index_num, value);
    } else {
        // roll input: [q0,q1,q2,...] ->
        // [q_half,q_half+1,...,q_end,q0,q1,...q_half-1]
        input_roll_buffer = roll_allocator.get();
        aclTensor* acl_input_roll_tensor = ggml_cann_create_tensor(
            input_roll_buffer, ggml_cann_type_mapping(src0->type),
            ggml_type_size(src0->type), src0->ne, src0->nb, GGML_MAX_DIMS);
        aclTensor* acl_input_tensor = ggml_cann_create_tensor(src0);

        int64_t shifts[] = {src0->ne[0] / 2};
        int64_t dims[] = {3};
        aclnn_roll(ctx, acl_input_tensor, acl_input_roll_tensor, shifts, dims);

        ACL_CHECK(aclDestroyTensor(acl_input_roll_tensor));
        ACL_CHECK(aclDestroyTensor(acl_input_tensor));

        // init [-1, -1, -1, 1, 1，1，...]
        minus_one_scale_buffer = minus_one_scale_allocator.get();

        int64_t minus_one_ne[4] = {src0->ne[0], 1, 1, 1};
        size_t minus_one_nb[GGML_MAX_DIMS];
        minus_one_nb[0] = sizeof(float_t);
        for (int i = 1; i < GGML_MAX_DIMS; i++) {
            minus_one_nb[i] = minus_one_nb[i - 1] * minus_one_ne[i - 1];
        }
        acl_minus_one_tensor = aclnn_ones(
            ctx, minus_one_scale_buffer, sizeof(float_t) * src0->ne[0],
            minus_one_ne, GGML_MAX_DIMS, ACL_FLOAT, sizeof(float_t), 1);
        // -1 * first half
        int64_t first_half_ne[4] = {src0->ne[0] / 2, 1, 1, 1};
        size_t first_half_nb[GGML_MAX_DIMS];
        first_half_nb[0] = sizeof(float_t);
        for (int i = 1; i < GGML_MAX_DIMS; i++) {
            first_half_nb[i] = first_half_nb[i - 1] * first_half_ne[i - 1];
        }
        aclTensor* acl_first_half_tensor = ggml_cann_create_tensor(
            minus_one_scale_buffer, ACL_FLOAT, sizeof(float_t), first_half_ne,
            first_half_nb, GGML_MAX_DIMS);
        bool inplace = true;
        float scale = -1;
        aclnn_muls(ctx, acl_first_half_tensor, scale, nullptr, inplace);
        ACL_CHECK(aclDestroyTensor(acl_first_half_tensor));
    }

    // TODO: n_dims < ne0
    GGML_ASSERT(n_dims == src0->ne[0]);

    // input * scale
    ggml_cann_pool_alloc roll_mul_scale_allocator(ctx.pool(),
                                                  ggml_nbytes(src0));
    void* input_roll_mul_scale_buffer = roll_mul_scale_allocator.get();
    size_t input_nb[GGML_MAX_DIMS];
    input_nb[0] = ggml_type_size(src0->type);
    for (int i = 1; i < GGML_MAX_DIMS; i++) {
        input_nb[i] = input_nb[i - 1] * src0->ne[i - 1];
    }
    aclTensor* acl_input_roll_mul_scale_tensor = ggml_cann_create_tensor(
        input_roll_mul_scale_buffer, ggml_cann_type_mapping(src0->type),
        ggml_type_size(src0->type), src0->ne, input_nb, GGML_MAX_DIMS);
    aclTensor* acl_input_roll_reshape_tensor = ggml_cann_create_tensor(
        input_roll_buffer, ggml_cann_type_mapping(src0->type),
        ggml_type_size(src0->type), src0->ne, input_nb, GGML_MAX_DIMS);

    aclnn_mul(ctx, acl_input_roll_reshape_tensor, acl_minus_one_tensor,
              acl_input_roll_mul_scale_tensor);

    // output
    aclTensor* acl_src0 = ggml_cann_create_tensor(src0);
    aclTensor* acl_dst = ggml_cann_create_tensor(dst);
    void* output_fp32_buffer;
    if (src0->type == GGML_TYPE_F32) {
        aclnn_inplace_mul(ctx, acl_src0, acl_cos_reshape_tensor);
        aclnn_inplace_mul(ctx, acl_input_roll_mul_scale_tensor,
                          acl_sin_reshape_tensor);
        aclnn_add(ctx, acl_src0, acl_input_roll_mul_scale_tensor, acl_dst);
        // TODO: ne0 != n_dims in mode2
    } else if (src0->type == GGML_TYPE_F16) {
        size_t input_fp32_nb[GGML_MAX_DIMS];
        input_fp32_nb[0] = sizeof(float_t);
        for (int i = 1; i < GGML_MAX_DIMS; i++) {
            input_fp32_nb[i] = input_fp32_nb[i - 1] * dst->ne[i - 1];
        }
        ggml_cann_pool_alloc fp32_allocator1(
            ctx.pool(), ggml_nelements(dst) * sizeof(float_t));
        void* input_fp32_buffer1 = fp32_allocator1.get();
        aclTensor* input_fp32_tensor1 = ggml_cann_create_tensor(
            input_fp32_buffer1, ACL_FLOAT, sizeof(float_t), dst->ne,
            input_fp32_nb, GGML_MAX_DIMS);
        ggml_cann_pool_alloc fp32_allocator2(
            ctx.pool(), ggml_nelements(dst) * sizeof(float_t));
        void* input_fp32_buffer2 = fp32_allocator2.get();
        aclTensor* input_fp32_tensor2 = ggml_cann_create_tensor(
            input_fp32_buffer2, ACL_FLOAT, sizeof(float_t), dst->ne,
            input_fp32_nb, GGML_MAX_DIMS);

        ggml_cann_pool_alloc fp32_allocator(
            ctx.pool(), ggml_nelements(dst) * sizeof(float_t));
        output_fp32_buffer = fp32_allocator.get();
        aclTensor* output_fp32_tensor = ggml_cann_create_tensor(
            output_fp32_buffer, ACL_FLOAT, sizeof(float_t), dst->ne,
            input_fp32_nb, GGML_MAX_DIMS);
        aclnn_mul(ctx, acl_src0, acl_cos_reshape_tensor, input_fp32_tensor1);
        aclnn_mul(ctx, acl_input_roll_mul_scale_tensor, acl_sin_reshape_tensor,
                  input_fp32_tensor2);
        aclnn_add(ctx, input_fp32_tensor1, input_fp32_tensor2,
                  output_fp32_tensor);
        aclnn_cast(ctx, output_fp32_tensor, acl_dst, ACL_FLOAT16);

        ACL_CHECK(aclDestroyTensor(input_fp32_tensor1));
        ACL_CHECK(aclDestroyTensor(input_fp32_tensor2));
        ACL_CHECK(aclDestroyTensor(output_fp32_tensor));
    }

    ACL_CHECK(aclDestroyTensor(acl_sin_reshape_tensor));
    ACL_CHECK(aclDestroyTensor(acl_cos_reshape_tensor));
    ACL_CHECK(aclDestroyTensor(acl_minus_one_tensor));
    ACL_CHECK(aclDestroyTensor(acl_input_roll_mul_scale_tensor));
    ACL_CHECK(aclDestroyTensor(acl_input_roll_reshape_tensor));
    ACL_CHECK(aclDestroyTensor(acl_src0));
    ACL_CHECK(aclDestroyTensor(acl_dst));
}
