#!/bin/bash
#
# Synchronize ggml changes to whisper.cpp
#
# Usage:
#
#   $ cd /path/to/whisper.cpp
#   $ ./extra/sync-ggml-am.sh -skip hash0,hash1,hash2...
#

set -e

sd=$(dirname $0)
cd $sd/../

SRC_WHISPER=$(pwd)
SRC_GGML=$(cd ../ggml; pwd)

if [ ! -d $SRC_GGML ]; then
    echo "ggml not found at $SRC_GGML"
    exit 1
fi

lc=$(cat $SRC_WHISPER/extra/sync-ggml.last)
echo "Syncing ggml changes since commit $lc"

to_skip=""
if [ "$1" == "-skip" ]; then
    to_skip=$2
fi

cd $SRC_GGML

git log --oneline $lc..HEAD
git log --oneline $lc..HEAD --reverse | grep -v "(whisper/[0-9]*)" | cut -d' ' -f1 > $SRC_WHISPER/ggml-commits

if [ ! -s $SRC_WHISPER/ggml-commits ]; then
    rm -v $SRC_WHISPER/ggml-commits
    echo "No new commits"
    exit 0
fi

if [ -f $SRC_WHISPER/ggml-src.patch ]; then
    rm -v $SRC_WHISPER/ggml-src.patch
fi

while read c; do
    if [ -n "$to_skip" ]; then
        if [[ $to_skip == *"$c"* ]]; then
            echo "Skipping $c"
            continue
        fi
    fi

    git format-patch -k $c~1..$c --stdout -- \
        include/ggml/ggml*.h \
        src/ggml*.h \
        src/ggml*.c \
        src/ggml*.cpp \
        src/ggml*.m \
        src/ggml*.metal \
        src/ggml*.cu \
        examples/common.h \
        examples/common.cpp \
        examples/common-ggml.h \
        examples/common-ggml.cpp \
        examples/whisper/whisper.h \
        examples/whisper/whisper.cpp \
        examples/whisper/main.cpp \
        examples/whisper/quantize.cpp \
        >> $SRC_WHISPER/ggml-src.patch
done < $SRC_WHISPER/ggml-commits

rm -v $SRC_WHISPER/ggml-commits

# delete files if empty
if [ ! -s $SRC_WHISPER/ggml-src.patch ]; then
    rm -v $SRC_WHISPER/ggml-src.patch
fi

cd $SRC_WHISPER

if [ -f $SRC_WHISPER/ggml-src.patch ]; then
    # replace PR numbers
    #
    # Subject: some text (#1234)
    # Subject: some text (ggml/1234)
    cat ggml-src.patch | sed -e 's/^Subject: \(.*\) (#\([0-9]*\))/Subject: \1 (ggml\/\2)/' > ggml-src.patch.tmp
    mv ggml-src.patch.tmp ggml-src.patch

    cat ggml-src.patch | sed -e 's/^\(.*\) (#\([0-9]*\))$/\1 (ggml\/\2)/' > ggml-src.patch.tmp
    mv ggml-src.patch.tmp ggml-src.patch

    # replace filenames:
    #
    # src/ggml.c                  -> ggml.c
    # src/ggml-alloc.c            -> ggml-alloc.c
    # src/ggml-backend-impl.h     -> ggml-backend-impl.h
    # src/ggml-backend.c          -> ggml-backend.c
    # src/ggml-common.h           -> ggml-common.h
    # src/ggml-cuda.cu            -> ggml-cuda.cu
    # src/ggml-cuda.h             -> ggml-cuda.h
    # src/ggml-impl.h             -> ggml-impl.h
    # src/ggml-kompute.cpp        -> ggml-kompute.cpp
    # src/ggml-kompute.h          -> ggml-kompute.h
    # src/ggml-metal.h            -> ggml-metal.h
    # src/ggml-metal.m            -> ggml-metal.m
    # src/ggml-mpi.h              -> ggml-mpi.h
    # src/ggml-mpi.c              -> ggml-mpi.c
    # src/ggml-opencl.cpp         -> ggml-opencl.cpp
    # src/ggml-opencl.h           -> ggml-opencl.h
    # src/ggml-quants.c           -> ggml-quants.c
    # src/ggml-quants.h           -> ggml-quants.h
    # src/ggml-sycl.cpp           -> ggml-sycl.cpp
    # src/ggml-sycl.h             -> ggml-sycl.h
    # src/ggml-vulkan.cpp         -> ggml-vulkan.cpp
    # src/ggml-vulkan.h           -> ggml-vulkan.h
    # include/ggml/ggml.h         -> ggml.h
    # include/ggml/ggml-alloc.h   -> ggml-alloc.h
    # include/ggml/ggml-backend.h -> ggml-backend.h
    #
    # examples/common.h           -> examples/common.h
    # examples/common.cpp         -> examples/common.cpp
    # examples/common-ggml.h      -> examples/common-ggml.h
    # examples/common-ggml.cpp    -> examples/common-ggml.cpp
    #
    # examples/whisper/whisper.h    -> whisper.h
    # examples/whisper/whisper.cpp  -> whisper.cpp
    # examples/whisper/main.cpp     -> examples/main/main.cpp
    # examples/whisper/quantize.cpp -> examples/quantize/quantize.cpp

    cat ggml-src.patch | sed \
        -e 's/src\/ggml\.c/ggml.c/g' \
        -e 's/src\/ggml-alloc\.c/ggml-alloc.c/g' \
        -e 's/src\/ggml-backend-impl\.h/ggml-backend-impl.h/g' \
        -e 's/src\/ggml-backend\.c/ggml-backend.c/g' \
        -e 's/src\/ggml-common\.h/ggml-common.h/g' \
        -e 's/src\/ggml-cuda\.cu/ggml-cuda.cu/g' \
        -e 's/src\/ggml-cuda\.h/ggml-cuda.h/g' \
        -e 's/src\/ggml-impl\.h/ggml-impl.h/g' \
        -e 's/src\/ggml-kompute\.cpp/ggml-kompute.cpp/g' \
        -e 's/src\/ggml-kompute\.h/ggml-kompute.h/g' \
        -e 's/src\/ggml-metal\.h/ggml-metal.h/g' \
        -e 's/src\/ggml-metal\.m/ggml-metal.m/g' \
        -e 's/src\/ggml-mpi\.h/ggml-mpi.h/g' \
        -e 's/src\/ggml-mpi\.c/ggml-mpi.c/g' \
        -e 's/src\/ggml-opencl\.cpp/ggml-opencl.cpp/g' \
        -e 's/src\/ggml-opencl\.h/ggml-opencl.h/g' \
        -e 's/src\/ggml-quants\.c/ggml-quants.c/g' \
        -e 's/src\/ggml-quants\.h/ggml-quants.h/g' \
        -e 's/src\/ggml-sycl\.cpp/ggml-sycl.cpp/g' \
        -e 's/src\/ggml-sycl\.h/ggml-sycl.h/g' \
        -e 's/src\/ggml-vulkan\.cpp/ggml-vulkan.cpp/g' \
        -e 's/src\/ggml-vulkan\.h/ggml-vulkan.h/g' \
        -e 's/include\/ggml\/ggml\.h/ggml.h/g' \
        -e 's/include\/ggml\/ggml-alloc\.h/ggml-alloc.h/g' \
        -e 's/include\/ggml\/ggml-backend\.h/ggml-backend.h/g' \
        -e 's/examples\/common\.h/examples\/common.h/g' \
        -e 's/examples\/common\.cpp/examples\/common.cpp/g' \
        -e 's/examples\/common-ggml\.h/examples\/common-ggml.h/g' \
        -e 's/examples\/common-ggml\.cpp/examples\/common-ggml.cpp/g' \
        -e 's/examples\/whisper\/whisper\.h/whisper.h/g' \
        -e 's/examples\/whisper\/whisper\.cpp/whisper.cpp/g' \
        -e 's/examples\/whisper\/main\.cpp/examples\/main\/main.cpp/g' \
        -e 's/examples\/whisper\/quantize\.cpp/examples\/quantize\/quantize.cpp/g' \
        > ggml-src.patch.tmp
    mv ggml-src.patch.tmp ggml-src.patch

    git am ggml-src.patch

    rm -v $SRC_WHISPER/ggml-src.patch
fi

# update last commit
cd $SRC_GGML
git log -1 --format=%H > $SRC_WHISPER/extra/sync-ggml.last

echo "Done"

exit 0
