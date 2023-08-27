cmake_minimum_required(VERSION 3.10)

project(whisper.cpp)

set(CMAKE_CXX_STANDARD 11)
set(WHISPER_LIB_DIR ${CMAKE_SOURCE_DIR}/../../../../../../../)

set(
        SOURCE_FILES
        ${WHISPER_LIB_DIR}/ggml.c
        ${WHISPER_LIB_DIR}/whisper.cpp
        ${CMAKE_SOURCE_DIR}/jni.c
)

if (${ANDROID_ABI} STREQUAL "arm64-v8a")
    set(WHISPER_LIBRARY_NAME whisper_v8fp16_va)
elseif (${ANDROID_ABI} STREQUAL "armeabi-v7a")
    set(WHISPER_LIBRARY_NAME whisper_vfpv4)
endif ()

add_library(
        ${WHISPER_LIBRARY_NAME}
        SHARED
        ${SOURCE_FILES}
)

if (${ANDROID_ABI} STREQUAL "arm64-v8a")
    target_compile_options(${WHISPER_LIBRARY_NAME} PRIVATE -march=armv8.2-a+fp16)
elseif (${ANDROID_ABI} STREQUAL "armeabi-v7a")
    target_compile_options(${WHISPER_LIBRARY_NAME} PRIVATE -mfpu=neon-vfpv4)
endif ()


target_link_libraries(${WHISPER_LIBRARY_NAME} log android)
include_directories(${WHISPER_LIB_DIR})

if (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    target_compile_options(${WHISPER_LIBRARY_NAME} PRIVATE -O3)
    target_compile_options(${WHISPER_LIBRARY_NAME} PRIVATE -fvisibility=hidden -fvisibility-inlines-hidden)
    target_compile_options(${WHISPER_LIBRARY_NAME} PRIVATE -ffunction-sections -fdata-sections)
    target_link_options(${WHISPER_LIBRARY_NAME} PRIVATE -Wl,--gc-sections)
    target_link_options(${WHISPER_LIBRARY_NAME} PRIVATE -Wl,--exclude-libs,ALL)
    target_link_options(${WHISPER_LIBRARY_NAME} PRIVATE -flto)
endif ()
