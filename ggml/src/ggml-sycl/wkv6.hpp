#ifndef GGML_SYCL_WKV6_HPP
#define GGML_SYCL_WKV6_HPP

#include "common.hpp"

void ggml_sycl_op_rwkv_wkv6(ggml_backend_sycl_context & ctx, const ggml_tensor *src0,
    const ggml_tensor *src1, ggml_tensor * dst);


#endif // GGML_SYCL_WKV6_HPP
