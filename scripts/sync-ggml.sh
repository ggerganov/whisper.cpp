#!/bin/bash

cp -rpv ../ggml/CMakeLists.txt       ./ggml/CMakeLists.txt
cp -rpv ../ggml/src/CMakeLists.txt   ./ggml/src/CMakeLists.txt
cp -rpv ../ggml/cmake/FindSIMD.cmake ./ggml/cmake/FindSIMD.cmake

cp -rpv ../ggml/src/ggml*.c          ./ggml/src/
cp -rpv ../ggml/src/ggml*.cpp        ./ggml/src/
cp -rpv ../ggml/src/ggml*.h          ./ggml/src/
cp -rpv ../ggml/src/ggml*.cu         ./ggml/src/
cp -rpv ../ggml/src/ggml*.m          ./ggml/src/
cp -rpv ../ggml/src/ggml-amx/*       ./ggml/src/ggml-amx/
cp -rpv ../ggml/src/ggml-cann/*      ./ggml/src/ggml-cann/
cp -rpv ../ggml/src/ggml-cuda/*      ./ggml/src/ggml-cuda/
cp -rpv ../ggml/src/ggml-sycl/*      ./ggml/src/ggml-sycl/
cp -rpv ../ggml/src/vulkan-shaders/* ./ggml/src/vulkan-shaders/

cp -rpv ../ggml/include/ggml*.h ./ggml/include/

cp -rpv ../ggml/examples/common.h        ./examples/common.h
cp -rpv ../ggml/examples/common.cpp      ./examples/common.cpp
cp -rpv ../ggml/examples/common-ggml.h   ./examples/common-ggml.h
cp -rpv ../ggml/examples/common-ggml.cpp ./examples/common-ggml.cpp

cp -rpv ../ggml/LICENSE                ./LICENSE
cp -rpv ../ggml/scripts/gen-authors.sh ./scripts/gen-authors.sh
