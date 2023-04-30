cmake_minimum_required (VERSION 3.0)

project(whisper.cpp VERSION 1.4.0)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    add_compile_options(/utf-8)
endif ()

# Add path to modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(WHISPER_STANDALONE ON)
    include(GitVars)
    include(BuildTypes)

    # configure project version
    if (EXISTS "${CMAKE_SOURCE_DIR}/bindings/ios/Makefile-tmpl")
        configure_file(${CMAKE_SOURCE_DIR}/bindings/ios/Makefile-tmpl ${CMAKE_SOURCE_DIR}/bindings/ios/Makefile @ONLY)
    endif()
    configure_file(${CMAKE_SOURCE_DIR}/bindings/javascript/package-tmpl.json ${CMAKE_SOURCE_DIR}/bindings/javascript/package.json @ONLY)
else()
    set(WHISPER_STANDALONE OFF)
endif()

if (EMSCRIPTEN)
    set(BUILD_SHARED_LIBS_DEFAULT OFF)

    option(WHISPER_WASM_SINGLE_FILE "whisper: embed WASM inside the generated whisper.js" ON)
else()
    if (MINGW)
        set(BUILD_SHARED_LIBS_DEFAULT OFF)
    else()
        set(BUILD_SHARED_LIBS_DEFAULT ON)
    endif()
endif()

# options

option(BUILD_SHARED_LIBS              "whisper: build shared libs" ${BUILD_SHARED_LIBS_DEFAULT})

option(WHISPER_ALL_WARNINGS           "whisper: enable all compiler warnings"                   ON)
option(WHISPER_ALL_WARNINGS_3RD_PARTY "whisper: enable all compiler warnings in 3rd party libs" OFF)

option(WHISPER_SANITIZE_THREAD        "whisper: enable thread sanitizer"    OFF)
option(WHISPER_SANITIZE_ADDRESS       "whisper: enable address sanitizer"   OFF)
option(WHISPER_SANITIZE_UNDEFINED     "whisper: enable undefined sanitizer" OFF)

option(WHISPER_BUILD_TESTS            "whisper: build tests"    ${WHISPER_STANDALONE})
option(WHISPER_BUILD_EXAMPLES         "whisper: build examples" ${WHISPER_STANDALONE})

option(WHISPER_SDL2                   "whisper: support for libSDL2" OFF)

if (APPLE)
    option(WHISPER_NO_ACCELERATE         "whisper: disable Accelerate framework" OFF)
    option(WHISPER_NO_AVX                "whisper: disable AVX" OFF)
    option(WHISPER_NO_AVX2               "whisper: disable AVX2" OFF)
    option(WHISPER_NO_FMA                "whisper: disable FMA" OFF)

    option(WHISPER_COREML                "whisper: enable Core ML framework" OFF)
    option(WHISPER_COREML_ALLOW_FALLBACK "whisper: allow non-CoreML fallback" OFF)
else()
    option(WHISPER_OPENBLAS              "whisper: support for OpenBLAS" OFF)
    option(WHISPER_CUBLAS                "whisper: support for cuBLAS" OFF)
endif()

option(WHISPER_PERF "whisper: enable perf timings" OFF)

# sanitizers

if (NOT MSVC)
    if (WHISPER_SANITIZE_THREAD)
        set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -fsanitize=thread")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
    endif()

    if (WHISPER_SANITIZE_ADDRESS)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}     -fsanitize=address -fno-omit-frame-pointer")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    endif()

    if (WHISPER_SANITIZE_UNDEFINED)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}     -fsanitize=undefined")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
    endif()
endif()

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffast-math")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native")

# dependencies

find_package(Threads REQUIRED)

# on APPLE
if (APPLE)
    # include Accelerate framework
    if (NOT WHISPER_NO_ACCELERATE)
        find_library(ACCELERATE_FRAMEWORK Accelerate)

        if (ACCELERATE_FRAMEWORK)
            message(STATUS "Accelerate framework found")

            set(WHISPER_EXTRA_LIBS  ${WHISPER_EXTRA_LIBS}  ${ACCELERATE_FRAMEWORK})
            set(WHISPER_EXTRA_FLAGS ${WHISPER_EXTRA_FLAGS} -DGGML_USE_ACCELERATE)
        else()
            message(WARNING "Accelerate framework not found")
        endif()
    endif()

    if (WHISPER_COREML)
        find_library(FOUNDATION_FRAMEWORK Foundation)
        find_library(COREML_FRAMEWORK CoreML)

        if (COREML_FRAMEWORK)
            message(STATUS "CoreML framework found")

            set(WHISPER_EXTRA_FLAGS ${WHISPER_EXTRA_FLAGS} -DWHISPER_USE_COREML)
        else()
            message(WARNING "CoreML framework not found")
        endif()

        if (WHISPER_COREML_ALLOW_FALLBACK)
            set(WHISPER_EXTRA_FLAGS ${WHISPER_EXTRA_FLAGS} -DWHISPER_USE_COREML_ALLOW_FALLBACK)
        endif()
    endif()
endif()

if (WHISPER_OPENBLAS)
    find_library(OPENBLAS_LIB
        NAMES openblas libopenblas
        )
    if (OPENBLAS_LIB)
        message(STATUS "OpenBLAS found")

        set(WHISPER_EXTRA_LIBS  ${WHISPER_EXTRA_LIBS}  ${OPENBLAS_LIB})
        set(WHISPER_EXTRA_FLAGS ${WHISPER_EXTRA_FLAGS} -DGGML_USE_OPENBLAS)
    else()
        message(WARNING "OpenBLAS not found")
    endif()
endif()

if (WHISPER_CUBLAS)
    cmake_minimum_required(VERSION 3.17)

    find_package(CUDAToolkit)

    if (CUDAToolkit_FOUND)
        message(STATUS "cuBLAS found")

        enable_language(CUDA)

        set(GGML_CUDA_SOURCES ggml-cuda.cu ggml-cuda.h)

        add_compile_definitions(GGML_USE_CUBLAS)

        if (WHISPER_STATIC)
            set(WHISPER_EXTRA_LIBS ${WHISPER_EXTRA_LIBS} CUDA::cudart_static CUDA::cublas_static CUDA::cublasLt_static)
        else()
            set(WHISPER_EXTRA_LIBS ${WHISPER_EXTRA_LIBS} CUDA::cudart CUDA::cublas CUDA::cublasLt)
        endif()

    else()
        message(WARNING "cuBLAS not found")
    endif()
endif()

# compiler flags

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo")
endif ()

if (WHISPER_ALL_WARNINGS)
    if (NOT MSVC)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
            -Wall                           \
            -Wextra                         \
            -Wpedantic                      \
            -Wshadow                        \
            -Wcast-qual                     \
            -Wstrict-prototypes             \
            -Wpointer-arith                 \
            -Wno-unused-function            \
        ")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
            -Wall                           \
            -Wextra                         \
            -Wpedantic                      \
            -Wcast-qual                     \
        ")
    else()
        # todo : msvc
    endif()
endif()

if (NOT MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror=vla")
    #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-math-errno -ffinite-math-only -funsafe-math-optimizations")
endif()

message(STATUS "CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")

if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm" OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
    message(STATUS "ARM detected")
else()
    message(STATUS "x86 detected")
    if (MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /arch:AVX2")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:AVX2")
    else()
        if (EMSCRIPTEN)
            set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -pthread")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
        else()
            if(NOT WHISPER_NO_AVX)
                set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mavx")
            endif()
            if(NOT WHISPER_NO_AVX2)
                set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mavx2")
            endif()
            if(NOT WHISPER_NO_FMA)
                set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfma")
            endif()
            if(NOT WHISPER_NO_F16C)
                set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mf16c")
            endif()
        endif()
    endif()
endif()

if (WHISPER_PERF)
    set(WHISPER_EXTRA_FLAGS ${WHISPER_EXTRA_FLAGS} -DGGML_PERF)
endif()

#
# whisper.coreml - Core ML support
#

if (WHISPER_COREML)
    set(TARGET whisper.coreml)

    add_library(${TARGET}
        coreml/whisper-encoder.h
        coreml/whisper-encoder.mm
        coreml/whisper-encoder-impl.h
        coreml/whisper-encoder-impl.m
        )

    include(DefaultTargetOptions)

    target_include_directories(${TARGET} PUBLIC
        .
        )

    target_link_libraries(${TARGET} PRIVATE ${FOUNDATION_FRAMEWORK} ${COREML_FRAMEWORK})

    set_target_properties(${TARGET} PROPERTIES
        COMPILE_FLAGS "-fobjc-arc"
        )
endif()

#
# whisper - this is the main library of the project
#

set(TARGET whisper)

add_library(${TARGET}
    ggml.h
    ggml.c
    ${GGML_CUDA_SOURCES}
    whisper.h
    whisper.cpp
    )

include(DefaultTargetOptions)

target_include_directories(${TARGET} PUBLIC
    .
    )

if (WHISPER_COREML)
    target_link_libraries(${TARGET} PRIVATE whisper.coreml)
endif()

if (MSVC)
    target_link_libraries(${TARGET} PRIVATE ${WHISPER_EXTRA_LIBS} ${CMAKE_THREAD_LIBS_INIT})

    set(WHISPER_EXTRA_FLAGS ${WHISPER_EXTRA_FLAGS} -D_CRT_SECURE_NO_WARNINGS)
else()
    target_link_libraries(${TARGET} PRIVATE m ${WHISPER_EXTRA_LIBS} ${CMAKE_THREAD_LIBS_INIT})
endif()

if (BUILD_SHARED_LIBS)
    target_link_libraries(${TARGET} PUBLIC
        ${CMAKE_DL_LIBS}
        )

    target_compile_definitions(${TARGET} PUBLIC
        WHISPER_SHARED
        GGML_SHARED
        )

    target_compile_definitions(${TARGET} PRIVATE
        WHISPER_BUILD
        GGML_BUILD
        )
endif()

if (GGML_CUDA_SOURCES)
    message(STATUS "GGML CUDA sources found, configuring CUDA architecture")
    set_property(TARGET whisper PROPERTY CUDA_ARCHITECTURES OFF)
    set_property(TARGET whisper PROPERTY CUDA_SELECT_NVCC_ARCH_FLAGS "Auto")
endif()

if (EMSCRIPTEN)
    set_target_properties(${TARGET} PROPERTIES COMPILE_FLAGS "-msimd128")
endif()

target_compile_definitions(${TARGET} PUBLIC
    ${WHISPER_EXTRA_FLAGS}
    )

set_target_properties(${TARGET} PROPERTIES PUBLIC_HEADER "whisper.h")

install(TARGETS ${TARGET}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib/static
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include
    )

#
# bindings
#

add_subdirectory(bindings)

#
# programs, examples and tests
#

if (WHISPER_BUILD_TESTS AND NOT CMAKE_JS_VERSION)
    enable_testing()
    add_subdirectory(tests)
endif ()

if (WHISPER_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
