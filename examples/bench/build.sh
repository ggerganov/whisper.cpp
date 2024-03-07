#!/bin/bash

# generate bench tool for Android-based device

set -e

TARGET=bench
BUILD_TYPE=Release

if [ -z ${ANDROID_NDK} ]; then
    echo -e "ANDROID_NDK not set"
    echo -e "like this:\n"
    echo -e "export ANDROID_NDK=/opt/android-ndk-r21e"
    exit 1
fi

if [ ! -d ${ANDROID_NDK} ]; then
    echo -e "pls set ANDROID_NDK properly"
    echo -e "like this:\n"
    echo -e "export ANDROID_NDK=/opt/android-ndk-r21e"
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

build_arm64
build_armv7a
