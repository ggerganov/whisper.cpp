#  MIT license
#  Copyright (C) 2024 Intel Corporation
#  SPDX-License-Identifier: MIT

mkdir -p build
cd build
source /opt/intel/oneapi/setvars.sh

#for FP16
#cmake .. -DGGML_SYCL=ON -DCMAKE_C_COMPILER=icx -DCMAKE_CXX_COMPILER=icpx -DWHISPER_SYCL_F16=ON # faster for long-prompt inference

#for FP32
cmake .. -DGGML_SYCL=ON -DCMAKE_C_COMPILER=icx -DCMAKE_CXX_COMPILER=icpx

#for other features from the examples, e.g. stream and talk link with SDL2:
#cmake .. -DGGML_SYCL=ON -DWHISPER_SDL2=ON -DCMAKE_C_COMPILER=icx -DCMAKE_CXX_COMPILER=icpx

#build example/main only
#cmake --build . --config Release --target main

#build all binary
cmake --build . --config Release -v
