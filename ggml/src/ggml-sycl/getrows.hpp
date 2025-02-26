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

#ifndef GGML_SYCL_GETROWS_HPP
#define GGML_SYCL_GETROWS_HPP

#include "common.hpp"

void ggml_sycl_op_get_rows(ggml_backend_sycl_context & ctx, const ggml_tensor *src0,
    const ggml_tensor *src1, ggml_tensor *dst,
    const float *src0_d, const float *src1_d,
    float *dst_d, const queue_ptr &stream);

#endif // GGML_SYCL_GETROWS_HPP
