#!/bin/bash

# generate bench tool for target x86-linux and target Android

set -e

TARGET=bench
BUILD_TYPE=Release
#BUILD_TYPE=Debug


if [ -z ${ANDROID_NDK} ]; then
    echo "ANDROID_NDK not set"
    exit 1
fi

if [ ! -d ${ANDROID_NDK} ]; then
    echo "pls set ANDROID_NDK properly"
    exit 1
fi

if [ -d out ]; then
    echo "remove out directory in `pwd`"
    rm -rf out
fi


function build_arm64
{
cmake -H. -B./out/arm64-v8a -DTARGET_NAME=${TARGET} -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DBUILD_TARGET="android" -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-21 -DANDROID_NDK=${ANDROID_NDK}  -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake
cd ./out/arm64-v8a
make

ls -l ${TARGET}
/bin/cp -fv ${TARGET} ../../${TARGET}_arm64
cd -
}


function build_armv7a
{
cmake -H. -B./out/armeabi-v7a -DTARGET_NAME=${TARGET} -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DBUILD_TARGET="android" -DANDROID_ABI=armeabi-v7a -DANDROID_PLATFORM=android-21 -DANDROID_NDK=${ANDROID_NDK}  -DCMAKE_TOOLCHAIN_FILE=${ANDROID_NDK}/build/cmake/android.toolchain.cmake
cd ./out/armeabi-v7a
make

ls -l ${TARGET}
/bin/cp -fv ${TARGET} ../../${TARGET}_armv7a
cd -
}


function build_x86
{
cmake -H. -B./out/x86 -DTARGET_NAME=${TARGET} -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DBUILD_TARGET="x86"
cd ./out/x86
make

ls -l ${TARGET}
/bin/cp -fv ${TARGET} ../../${TARGET}_x86
cd -
}

build_arm64
build_armv7a
build_x86
