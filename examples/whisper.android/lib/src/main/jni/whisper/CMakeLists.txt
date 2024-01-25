cmake_minimum_required(VERSION 3.10)

project(whisper.cpp)

set(CMAKE_CXX_STANDARD 11)
set(WHISPER_LIB_DIR ${CMAKE_SOURCE_DIR}/../../../../../../..)

set(
        SOURCE_FILES
        ${WHISPER_LIB_DIR}/whisper.cpp
        ${CMAKE_SOURCE_DIR}/jni.c
)

option(
        GGML_HOME
        "Path to GGML"
        ${WHISPER_LIB_DIR}/../ggml
)

# Building from Android doesn't seem to respect the default value, so we set it here explicitly.
if(NOT GGML_HOME)
    set(GGML_HOME ${WHISPER_LIB_DIR}/../ggml)
endif()

find_library(LOG_LIB log)

include(FetchContent)
FetchContent_Declare(ggml SOURCE_DIR ${GGML_HOME})
FetchContent_MakeAvailable(ggml)

function(build_library target_name)
    add_library(
        ${target_name}
        SHARED
        ${SOURCE_FILES}
    )

    target_link_libraries(${target_name} ${LOG_LIB} android ggml)

    if (${target_name} STREQUAL "whisper_v8fp16_va")
        target_compile_options(${target_name} PRIVATE -march=armv8.2-a+fp16)
    elseif (${target_name} STREQUAL "whisper_vfpv4")
        target_compile_options(${target_name} PRIVATE -mfpu=neon-vfpv4)
    endif ()

    if (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug")

        target_compile_options(${target_name} PRIVATE -O3)
        target_compile_options(${target_name} PRIVATE -fvisibility=hidden -fvisibility-inlines-hidden)
        target_compile_options(${target_name} PRIVATE -ffunction-sections -fdata-sections)

        target_link_options(${target_name} PRIVATE -Wl,--gc-sections)
        target_link_options(${target_name} PRIVATE -Wl,--exclude-libs,ALL)
        target_link_options(${target_name} PRIVATE -flto)

    endif ()
endfunction()

build_library("whisper") # Default target

if (${ANDROID_ABI} STREQUAL "arm64-v8a")
    build_library("whisper_v8fp16_va")
elseif (${ANDROID_ABI} STREQUAL "armeabi-v7a")
    build_library("whisper_vfpv4")
endif ()

include_directories(${WHISPER_LIB_DIR})
