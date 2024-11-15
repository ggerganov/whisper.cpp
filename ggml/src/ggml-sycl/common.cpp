//
// MIT license
// Copyright (C) 2024 Intel Corporation
// SPDX-License-Identifier: MIT
//

//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//

#include "common.hpp"

int get_current_device_id() {
  return dpct::dev_mgr::instance().current_device_id();
}

void* ggml_sycl_host_malloc(size_t size) try {
  if (getenv("GGML_SYCL_NO_PINNED") != nullptr) {
    return nullptr;
  }

  void* ptr = nullptr;
  // allow to use dpct::get_in_order_queue() for host malloc
  dpct::err0 err = CHECK_TRY_ERROR(
      ptr = (void*)sycl::malloc_host(size, dpct::get_in_order_queue()));

  if (err != 0) {
    // clear the error
    fprintf(
        stderr,
        "WARNING: failed to allocate %.2f MB of pinned memory: %s\n",
        size / 1024.0 / 1024.0,
        "syclGetErrorString is not supported");
    return nullptr;
  }

  return ptr;
} catch (sycl::exception const& exc) {
  std::cerr << exc.what() << "Exception caught at file:" << __FILE__
            << ", line:" << __LINE__ << std::endl;
  std::exit(1);
}

void ggml_sycl_host_free(void* ptr) try {
  // allow to use dpct::get_in_order_queue() for host malloc
  SYCL_CHECK(CHECK_TRY_ERROR(sycl::free(ptr, dpct::get_in_order_queue())));
} catch (sycl::exception const& exc) {
  std::cerr << exc.what() << "Exception caught at file:" << __FILE__
            << ", line:" << __LINE__ << std::endl;
  std::exit(1);
}

int64_t downsample_sycl_global_range(int64_t accumulate_block_num, int64_t block_size) {
  const int64_t max_range = std::numeric_limits<int>::max();
  int64_t sycl_down_blk_size = block_size;
  int64_t global_range = accumulate_block_num * sycl_down_blk_size;
  while(global_range > max_range) {
      sycl_down_blk_size /= 2;
      global_range = accumulate_block_num * sycl_down_blk_size;
  }
  return sycl_down_blk_size;
}

void ggml_sycl_op_flatten(ggml_backend_sycl_context & ctx, const ggml_tensor *src0,
                                 const ggml_tensor *src1, ggml_tensor *dst,
                                 const ggml_sycl_op_flatten_t op) try {
    const int64_t nrows0 = ggml_nrows(src0);

    const bool use_src1 = src1 != nullptr;
    const int64_t nrows1 = use_src1 ? ggml_nrows(src1) : 1;

    GGML_ASSERT(!use_src1 || src1->backend != GGML_BACKEND_TYPE_GPU_SPLIT);
    GGML_ASSERT(              dst->backend != GGML_BACKEND_TYPE_GPU_SPLIT);

    ggml_tensor_extra_gpu * src0_extra =            (ggml_tensor_extra_gpu *) src0->extra;
    ggml_tensor_extra_gpu * src1_extra = use_src1 ? (ggml_tensor_extra_gpu *) src1->extra : nullptr;
    ggml_tensor_extra_gpu * dst_extra  =            (ggml_tensor_extra_gpu *)  dst->extra;

    // dd = data device
    float * src0_ddf = (float *) src0->data;
    float * src1_ddf = use_src1 ? (float *) src1->data : nullptr;
    float *  dst_ddf = (float *) dst->data;

    ggml_sycl_pool_alloc<float> src0_f(ctx.pool());
    ggml_sycl_pool_alloc<float> src1_f(ctx.pool());
    ggml_sycl_pool_alloc<float>  dst_f(ctx.pool());

    ggml_sycl_set_device(ctx.device);
    queue_ptr main_stream = ctx.stream();
    // GGML_SYCL_DEBUG("ctx.device=%d, main_stream=%p src0_on_device=%d, src1_on_device=%d, dst_on_device=%d\n",
        // ctx.device, main_stream, src0_on_device, src1_on_device, dst_on_device);

    // do the computation
    op(ctx, src0, src1, dst, src0_ddf, src1_ddf, dst_ddf, main_stream);
    // print_ggml_tensor("tensor", dst);
}
catch (sycl::exception const &exc) {

  std::cerr << exc.what() << "Exception caught at file:" << __FILE__
            << ", line:" << __LINE__ << std::endl;
  std::exit(1);
}
