#!/bin/bash

cp -rpv ../ggml/src/ggml.c               ./ggml.c
cp -rpv ../ggml/src/ggml-cuda.h          ./ggml-cuda.h
cp -rpv ../ggml/src/ggml-cuda.cu         ./ggml-cuda.cu
cp -rpv ../ggml/src/ggml-opencl.h        ./ggml-opencl.h
cp -rpv ../ggml/src/ggml-opencl.c        ./ggml-opencl.c
cp -rpv ../ggml/include/ggml/ggml.h      ./ggml.h
cp -rpv ../ggml/examples/common.h        ./examples/common.h
cp -rpv ../ggml/examples/common.cpp      ./examples/common.cpp
cp -rpv ../ggml/examples/common-ggml.h   ./examples/common-ggml.h
cp -rpv ../ggml/examples/common-ggml.cpp ./examples/common-ggml.cpp
