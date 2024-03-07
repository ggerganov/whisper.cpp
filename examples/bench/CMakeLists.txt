cmake_minimum_required(VERSION 3.10)

project(bench)
set(CMAKE_VERBOSE_MAKEFILE on  )

set(TARGET             ${TARGET_NAME})
set(WHISPERCPP_PATH    ${PROJECT_SOURCE_DIR}/../../)

message("target         : "  ${TARGET})
message("whispercpp path: "  ${WHISPERCPP_PATH})

IF (${BUILD_TARGET} MATCHES "android")
    add_definitions(-DTARGET_ANDROID)
    add_definitions(-D__ARM_NEON)
    link_libraries(log)
ENDIF()


IF (${BUILD_TARGET} MATCHES "x86")
    add_definitions(-D__gnu_linux__)
    add_definitions(-D_GNU_SOURCE)
    link_libraries(pthread m)
ENDIF()

include_directories(${WHISPERCPP_PATH}/)

add_executable(${TARGET} bench.cpp
    ${WHISPERCPP_PATH}/ggml.c
    ${WHISPERCPP_PATH}/ggml-alloc.c
    ${WHISPERCPP_PATH}/ggml-backend.c
    ${WHISPERCPP_PATH}/ggml-quants.c
    ${WHISPERCPP_PATH}/whisper.cpp
)
