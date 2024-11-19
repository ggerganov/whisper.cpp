#!/bin/bash
#
# Synchronize ggml changes to whisper.cpp
#
# Usage:
#
#   $ cd /path/to/whisper.cpp
#   $ ./scripts/sync-ggml-am.sh -skip hash0,hash1,hash2...
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

lc=$(cat $SRC_WHISPER/scripts/sync-ggml.last)
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
        CMakeLists.txt \
        src/CMakeLists.txt \
        cmake/FindSIMD.cmake \
        src/ggml*.h \
        src/ggml*.c \
        src/ggml*.cpp \
        src/ggml-amx/* \
        src/ggml-blas/* \
        src/ggml-cann/* \
        src/ggml-cpu/* \
        src/ggml-cuda/* \
        src/ggml-hip/* \
        src/ggml-kompute/* \
        src/ggml-metal/* \
        src/ggml-musa/* \
        src/ggml-rpc/* \
        src/ggml-sycl/* \
        src/ggml-vulkan/* \
        include/ggml*.h \
        examples/common.h \
        examples/common.cpp \
        examples/common-ggml.h \
        examples/common-ggml.cpp \
        LICENSE \
        scripts/gen-authors.sh \
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
    # CMakelists.txt       -> ggml/CMakeLists.txt
    # src/CMakeLists.txt   -> ggml/src/CMakeLists.txt
    # cmake/FindSIMD.cmake -> ggml/cmake/FindSIMD.cmake
    #
    # src/ggml*.c          -> ggml/src/ggml*.c
    # src/ggml*.cpp        -> ggml/src/ggml*.cpp
    # src/ggml*.h          -> ggml/src/ggml*.h
    # src/ggml-amx/*       -> ggml/src/ggml-amx/*
    # src/ggml-blas/*      -> ggml/src/ggml-blas/*
    # src/ggml-cann/*      -> ggml/src/ggml-cann/*
    # src/ggml-cpu/*       -> ggml/src/ggml-cpu/*
    # src/ggml-cuda/*      -> ggml/src/ggml-cuda/*
    # src/ggml-hip/*       -> ggml/src/ggml-hip/*
    # src/ggml-kompute/*   -> ggml/src/ggml-kompute/*
    # src/ggml-metal/*     -> ggml/src/ggml-metal/*
    # src/ggml-musa/*      -> ggml/src/ggml-musa/*
    # src/ggml-rpc/*       -> ggml/src/ggml-rpc/*
    # src/ggml-sycl/*      -> ggml/src/ggml-sycl/*
    # src/ggml-vulkan/*    -> ggml/src/ggml-vulkan/*
    #
    # include/ggml*.h -> ggml/include/ggml*.h
    #
    # examples/common.h        -> examples/common.h
    # examples/common.cpp      -> examples/common.cpp
    # examples/common-ggml.h   -> examples/common-ggml.h
    # examples/common-ggml.cpp -> examples/common-ggml.cpp
    #
    # LICENSE                     -> LICENSE
    # ggml/scripts/gen-authors.sh -> scripts/gen-authors.sh

    cat ggml-src.patch | sed -E \
        -e 's/(^[[:space:]]|[ab]\/)CMakeLists.txt/\1ggml\/CMakeLists.txt/g' \
        -e 's/(^[[:space:]]|[ab]\/)src\/CMakeLists.txt/\1ggml\/src\/CMakeLists.txt/g' \
        -e 's/(^[[:space:]]|[ab]\/)cmake\/FindSIMD.cmake/\1ggml\/cmake\/FindSIMD.cmake/g' \
        -e 's/([[:space:]]|[ab]\/)src\/ggml(.*)\.c/\1ggml\/src\/ggml\2.c/g' \
        -e 's/([[:space:]]|[ab]\/)src\/ggml(.*)\.cpp/\1ggml\/src\/ggml\2.cpp/g' \
        -e 's/([[:space:]]|[ab]\/)src\/ggml(.*)\.h/\1ggml\/src\/ggml\2.h/g' \
        -e 's/([[:space:]]|[ab]\/)src\/ggml-amx\//\1ggml\/src\/ggml-amx\//g' \
        -e 's/([[:space:]]|[ab]\/)src\/ggml-blas\//\1ggml\/src\/ggml-blas\//g' \
        -e 's/([[:space:]]|[ab]\/)src\/ggml-cann\//\1ggml\/src\/ggml-cann\//g' \
        -e 's/([[:space:]]|[ab]\/)src\/ggml-cpu\//\1ggml\/src\/ggml-cpu\//g' \
        -e 's/([[:space:]]|[ab]\/)src\/ggml-cuda\//\1ggml\/src\/ggml-cuda\//g' \
        -e 's/([[:space:]]|[ab]\/)src\/ggml-hip\//\1ggml\/src\/ggml-hip\//g' \
        -e 's/([[:space:]]|[ab]\/)src\/ggml-kompute\//\1ggml\/src\/ggml-kompute\//g' \
        -e 's/([[:space:]]|[ab]\/)src\/ggml-metal\//\1ggml\/src\/ggml-metal\//g' \
        -e 's/([[:space:]]|[ab]\/)src\/ggml-musa\//\1ggml\/src\/ggml-musa\//g' \
        -e 's/([[:space:]]|[ab]\/)src\/ggml-rpc\//\1ggml\/src\/ggml-rpc\//g' \
        -e 's/([[:space:]]|[ab]\/)src\/ggml-sycl\//\1ggml\/src\/ggml-sycl\//g' \
        -e 's/([[:space:]]|[ab]\/)src\/ggml-vulkan\//\1ggml\/src\/ggml-vulkan\//g' \
        -e 's/([[:space:]]|[ab]\/)include\/ggml(.*)\.h/\1ggml\/include\/ggml\2.h/g' \
        -e 's/(^[[:space:]]|[ab]\/)examples\/common\.h/\1examples\/common.h/g' \
        -e 's/(^[[:space:]]|[ab]\/)examples\/common\.cpp/\1examples\/common.cpp/g' \
        -e 's/(^[[:space:]]|[ab]\/)examples\/common-ggml\.h/\1examples\/common-ggml.h/g' \
        -e 's/(^[[:space:]]|[ab]\/)examples\/common-ggml\.cpp/\1examples\/common-ggml.cpp/g' \
        -e 's/(^[[:space:]]|[ab]\/)LICENSE/\1LICENSE/g' \
        -e 's/(^[[:space:]]|[ab]\/)scripts\/gen-authors\.sh/\1scripts\/gen-authors.sh/g' \
        > ggml-src.patch.tmp
    mv ggml-src.patch.tmp ggml-src.patch

    git am ggml-src.patch

    rm -v $SRC_WHISPER/ggml-src.patch
fi

# update last commit
cd $SRC_GGML
git log -1 --format=%H > $SRC_WHISPER/scripts/sync-ggml.last

echo "Done"

exit 0
