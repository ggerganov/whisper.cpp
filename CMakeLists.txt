cmake_minimum_required (VERSION 3.5)

project(whisper.cpp VERSION 1.5.4)
set(SOVERSION 1)

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

if (APPLE)
    set(WHISPER_METAL_DEFAULT ON)
else()
    set(WHISPER_METAL_DEFAULT OFF)
endif()

option(BUILD_SHARED_LIBS              "whisper: build shared libs" ${BUILD_SHARED_LIBS_DEFAULT})

option(WHISPER_ALL_WARNINGS           "whisper: enable all compiler warnings"                   ON)
option(WHISPER_ALL_WARNINGS_3RD_PARTY "whisper: enable all compiler warnings in 3rd party libs" OFF)

option(WHISPER_SANITIZE_THREAD        "whisper: enable thread sanitizer"    OFF)
option(WHISPER_SANITIZE_ADDRESS       "whisper: enable address sanitizer"   OFF)
option(WHISPER_SANITIZE_UNDEFINED     "whisper: enable undefined sanitizer" OFF)

option(WHISPER_BUILD_TESTS            "whisper: build tests"    ${WHISPER_STANDALONE})
option(WHISPER_BUILD_EXAMPLES         "whisper: build examples" ${WHISPER_STANDALONE})

option(WHISPER_SDL2                   "whisper: support for libSDL2" OFF)

option(WHISPER_NO_AVX                 "whisper: disable AVX"  OFF)
option(WHISPER_NO_AVX2                "whisper: disable AVX2" OFF)
option(WHISPER_NO_FMA                 "whisper: disable FMA"  OFF)
option(WHISPER_NO_F16C                "whisper: disable F16c" OFF)

option(WHISPER_OPENVINO               "whisper: support for OpenVINO" OFF)

if (APPLE)
    option(WHISPER_NO_ACCELERATE         "whisper: disable Accelerate framework" OFF)
    option(WHISPER_METAL                 "whisper: use Metal"                    ${WHISPER_METAL_DEFAULT})
    option(WHISPER_METAL_NDEBUG          "whisper: disable Metal debugging"      OFF)
    option(WHISPER_COREML                "whisper: enable Core ML framework"     OFF)
    option(WHISPER_COREML_ALLOW_FALLBACK "whisper: allow non-CoreML fallback"    OFF)
    option(WHISPER_METAL_EMBED_LIBRARY   "whisper: embed Metal library"          OFF)
else()
    option(WHISPER_BLAS                  "whisper: use BLAS libraries"                        OFF)
    option(WHISPER_BLAS_VENDOR           "whisper: BLAS library vendor"                       Generic)
    option(WHISPER_OPENBLAS              "whisper: prefer OpenBLAS"                           OFF)
    option(WHISPER_CUBLAS                "whisper: support for cuBLAS"                        OFF)
    option(WHISPER_HIPBLAS               "whisper: support for hipBLAS"                       OFF)
    option(WHISPER_CLBLAST               "whisper: use CLBlast"                               OFF)
    option(WHISPER_SYCL                  "whisper: use SYCL"                                  OFF)
    option(WHISPER_SYCL_F16              "whisper: use 16 bit floats for sycl calculations"   OFF)
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

#compile flag sycl
if (WHISPER_SYCL)
    set(CMAKE_CXX_STANDARD 17)
else()
    set(CMAKE_CXX_STANDARD 11)
endif()

# on APPLE
if (APPLE)
    # include Accelerate framework
    if (NOT WHISPER_NO_ACCELERATE)
        find_library(ACCELERATE_FRAMEWORK Accelerate)

        if (ACCELERATE_FRAMEWORK)
            message(STATUS "Accelerate framework found")

            set(WHISPER_EXTRA_LIBS  ${WHISPER_EXTRA_LIBS}  ${ACCELERATE_FRAMEWORK})
            set(WHISPER_EXTRA_FLAGS ${WHISPER_EXTRA_FLAGS} -DGGML_USE_ACCELERATE -DACCELERATE_NEW_LAPACK -DACCELERATE_LAPACK_ILP64)
        else()
            message(FATAL_ERROR "Accelerate framework not found")
        endif()
    endif()

    if (WHISPER_METAL)
        find_library(FOUNDATION_LIBRARY         Foundation              REQUIRED)
        find_library(METAL_FRAMEWORK            Metal                   REQUIRED)
        find_library(METALKIT_FRAMEWORK         MetalKit                REQUIRED)

        if (METAL_FRAMEWORK)
            message(STATUS "Metal framework found")

            set(WHISPER_EXTRA_LIBS ${WHISPER_EXTRA_LIBS}
                ${FOUNDATION_LIBRARY}
                ${METAL_FRAMEWORK}
                ${METALKIT_FRAMEWORK}
                )
            set(WHISPER_EXTRA_FLAGS ${WHISPER_EXTRA_FLAGS} -DGGML_USE_METAL)

            if (WHISPER_METAL_NDEBUG)
                set(WHISPER_EXTRA_FLAGS ${WHISPER_EXTRA_FLAGS} -DGGML_METAL_NDEBUG)
            endif()
        else()
            message(FATAL_ERROR "Metal framework not found")
        endif()

        set(GGML_SOURCES_METAL ggml-metal.m ggml-metal.h)

        # copy ggml-common.h and ggml-metal.metal to bin directory
        configure_file(ggml-common.h    bin/ggml-common.h    COPYONLY)
        configure_file(ggml-metal.metal bin/ggml-metal.metal COPYONLY)

        if (WHISPER_METAL_EMBED_LIBRARY)
            enable_language(ASM)
            set(WHISPER_EXTRA_FLAGS ${WHISPER_EXTRA_FLAGS} -DGGML_METAL_EMBED_LIBRARY)

            set(METALLIB_SOURCE "${CMAKE_SOURCE_DIR}/ggml-metal.metal")

            file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/autogenerated")
            set(EMBED_METALLIB_ASSEMBLY "${CMAKE_BINARY_DIR}/autogenerated/ggml-embed-metallib.s")

            add_custom_command(
                OUTPUT ${EMBED_METALLIB_ASSEMBLY}
                COMMAND echo ".section __DATA,__ggml_metallib" > ${EMBED_METALLIB_ASSEMBLY}
                COMMAND echo ".globl _ggml_metallib_start" >> ${EMBED_METALLIB_ASSEMBLY}
                COMMAND echo "_ggml_metallib_start:" >> ${EMBED_METALLIB_ASSEMBLY}
                COMMAND echo ".incbin \\\"${METALLIB_SOURCE}\\\"" >> ${EMBED_METALLIB_ASSEMBLY}
                COMMAND echo ".globl _ggml_metallib_end" >> ${EMBED_METALLIB_ASSEMBLY}
                COMMAND echo "_ggml_metallib_end:" >> ${EMBED_METALLIB_ASSEMBLY}
                DEPENDS ${METALLIB_SOURCE}
                COMMENT "Generate assembly for embedded Metal library"
            )

            set(GGML_SOURCES_METAL ${GGML_SOURCES_METAL} ${EMBED_METALLIB_ASSEMBLY})
        endif()
    endif()

    if (WHISPER_COREML)
        find_library(FOUNDATION_FRAMEWORK Foundation)
        find_library(COREML_FRAMEWORK CoreML)

        if (COREML_FRAMEWORK)
            message(STATUS "CoreML framework found")

            set(WHISPER_EXTRA_FLAGS ${WHISPER_EXTRA_FLAGS} -DWHISPER_USE_COREML)
        else()
            message(FATAL_ERROR "CoreML framework not found")
        endif()

        if (WHISPER_COREML_ALLOW_FALLBACK)
            set(WHISPER_EXTRA_FLAGS ${WHISPER_EXTRA_FLAGS} -DWHISPER_COREML_ALLOW_FALLBACK)
        endif()
    endif()
endif()

if (WHISPER_OPENBLAS)
    set(WHISPER_BLAS_VENDOR "OpenBLAS")
    set(WHISPER_BLAS ON)
endif()

if (WHISPER_BLAS)
    if (WIN32)
        if(DEFINED ENV{OPENBLAS_PATH})
            set(BLAS_LIBRARIES $ENV{OPENBLAS_PATH}/lib/libopenblas.dll.a)
            message(STATUS "Libraries ${BLAS_LIBRARIES}")
            set(WHISPER_EXTRA_FLAGS ${WHISPER_EXTRA_FLAGS} -DGGML_USE_OPENBLAS)
            include_directories($ENV{OPENBLAS_PATH}/include)
            set(WHISPER_EXTRA_LIBS ${WHISPER_EXTRA_LIBS} ${BLAS_LIBRARIES})
        else ()
            message(FATAL_ERROR "BLAS library was not found. Environment variable OPENBLAS_PATH not defined.")
        endif ()
    else ()
        set(BLA_STATIC 1)
        set(BLA_VENDOR ${WHISPER_BLAS_VENDOR})
        set(BLA_SIZEOF_INTEGER 8)
        set(BLA_PREFER_PKGCONFIG 1)
        find_package(BLAS)

        if(BLAS_FOUND)
            message(STATUS "BLAS compatible library found")
            message(STATUS "Libraries ${BLAS_LIBRARIES}")
            find_path(BLAS_INCLUDE_DIRS cblas.h /usr/include/openblas /usr/local/include/openblas $ENV{BLAS_HOME}/include)
            set(WHISPER_EXTRA_FLAGS ${WHISPER_EXTRA_FLAGS} -DGGML_USE_OPENBLAS)
            include_directories(${BLAS_INCLUDE_DIRS})
            set(WHISPER_EXTRA_LIBS ${WHISPER_EXTRA_LIBS} ${BLAS_LIBRARIES})
        else()
            message(FATAL_ERROR "BLAS library was not found")
        endif()
    endif ()
endif ()

if (WHISPER_CUBLAS)
    cmake_minimum_required(VERSION 3.17)

    find_package(CUDAToolkit)

    if (CUDAToolkit_FOUND)
        message(STATUS "cuBLAS found")

        enable_language(CUDA)

        set(GGML_SOURCES_CUDA ggml-cuda.cu ggml-cuda.h)

        add_compile_definitions(GGML_USE_CUBLAS)

        if (WHISPER_STATIC)
            if (WIN32)
                # As of 12.3.1 CUDA Tookit for Windows does not offer a static cublas library
                set(WHISPER_EXTRA_LIBS ${WHISPER_EXTRA_LIBS} CUDA::cudart_static CUDA::cublas CUDA::cublasLt)
            else ()
                set(WHISPER_EXTRA_LIBS ${WHISPER_EXTRA_LIBS} CUDA::cudart_static CUDA::cublas_static CUDA::cublasLt_static)
            endif()
        else()
            set(WHISPER_EXTRA_LIBS ${WHISPER_EXTRA_LIBS} CUDA::cudart CUDA::cublas CUDA::cublasLt)
        endif()

        set(WHISPER_EXTRA_LIBS ${WHISPER_EXTRA_LIBS} CUDA::cuda_driver)
    else()
        message(FATAL_ERROR "cuBLAS not found")
    endif()
endif()


if (WHISPER_HIPBLAS)
    list(APPEND CMAKE_PREFIX_PATH /opt/rocm)
    if (NOT ${CMAKE_C_COMPILER_ID} MATCHES "Clang")
        message(WARNING "Only LLVM is supported for HIP, hint: CC=/opt/rocm/llvm/bin/clang")
    endif()
    if (NOT ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
        message(WARNING "Only LLVM is supported for HIP, hint: CXX=/opt/rocm/llvm/bin/clang++")
    endif()

    find_package(hip)
    find_package(hipblas)
    find_package(rocblas)

    if (${hipblas_FOUND} AND ${hip_FOUND})
        message(STATUS "HIP and hipBLAS found")
        add_compile_definitions(GGML_USE_HIPBLAS GGML_USE_CUBLAS)
        add_library(ggml-rocm OBJECT ggml-cuda.cu ggml-cuda.h)
        set_property(TARGET ggml-rocm PROPERTY POSITION_INDEPENDENT_CODE ON)
        set_source_files_properties(ggml-cuda.cu PROPERTIES LANGUAGE CXX)
        target_link_libraries(ggml-rocm PRIVATE hip::device PUBLIC hip::host roc::rocblas roc::hipblas)

        if (WHISPER_STATIC)
            message(FATAL_ERROR "Static linking not supported for HIP/ROCm")
        endif()
        set(WHISPER_EXTRA_LIBS ${WHISPER_EXTRA_LIBS} ggml-rocm)
    else()
        message(FATAL_ERROR "hipBLAS or HIP not found. Try setting CMAKE_PREFIX_PATH=/opt/rocm")
    endif()
endif()

if (WHISPER_CLBLAST)
    find_package(CLBlast)
    if (CLBlast_FOUND)
        message(STATUS "CLBlast found")

        set(GGML_SOURCES_OPENCL ggml-opencl.cpp ggml-opencl.h)

        add_compile_definitions(GGML_USE_CLBLAST)

        set(WHISPER_EXTRA_LIBS ${WHISPER_EXTRA_LIBS} clblast)
    else()
        message(FATAL_ERROR "CLBlast not found")
    endif()
endif()

if( WHISPER_OPENVINO )
    find_package(OpenVINO REQUIRED COMPONENTS Runtime)
endif()

if (WHISPER_SYCL)
    if ( NOT DEFINED ENV{ONEAPI_ROOT})
        message(FATAL_ERROR "Not detect ENV {ONEAPI_ROOT}, please install oneAPI & source it, like: source /opt/intel/oneapi/setvars.sh")
    endif()
    #todo: AOT

    find_package(IntelSYCL REQUIRED)
    if (WHISPER_SYCL_F16)
        add_compile_definitions(GGML_SYCL_F16)
    endif()
    add_compile_definitions(GGML_USE_SYCL)

    add_compile_options(-I./) #include DPCT
    add_compile_options(-I/${SYCL_INCLUDE_DIR})

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-narrowing")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsycl -L${MKLROOT}/lib")

    set(GGML_HEADERS_SYCL ggml-sycl.h)
    set(GGML_SOURCES_SYCL ggml-sycl.cpp)

    set(WHISPER_EXTRA_LIBS ${WHISPER_EXTRA_LIBS} sycl OpenCL mkl_core pthread m dl mkl_sycl_blas mkl_intel_ilp64 mkl_tbb_thread)
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
    # TODO: temporary disabled until we figure out ggml-metal.m
    #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror=vla")
    #set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-math-errno -ffinite-math-only -funsafe-math-optimizations")
endif()

message(STATUS "CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")

if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm" OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
    message(STATUS "ARM detected")
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "ppc64le")
    message(STATUS "PowerPC detected")
else()
    message(STATUS "x86 detected")
    if (MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /utf-8")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /utf-8")
        if(NOT WHISPER_NO_AVX2)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX2")
            set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /arch:AVX2")
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:AVX2")
        else()
            if(NOT WHISPER_NO_AVX)
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:AVX")
                set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /arch:AVX")
                set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:AVX")
            endif()
        endif()
    else()
        if (EMSCRIPTEN)
            set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -pthread -s TOTAL_STACK=5242880")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -s TOTAL_STACK=5242880")
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

#
# POSIX conformance
#

# clock_gettime came in POSIX.1b (1993)
# CLOCK_MONOTONIC came in POSIX.1-2001 / SUSv3 as optional
# posix_memalign came in POSIX.1-2001 / SUSv3
# M_PI is an XSI extension since POSIX.1-2001 / SUSv3, came in XPG1 (1985)
add_compile_definitions(_XOPEN_SOURCE=600)

# Somehow in OpenBSD whenever POSIX conformance is specified
# some string functions rely on locale_t availability,
# which was introduced in POSIX.1-2008, forcing us to go higher
if (CMAKE_SYSTEM_NAME MATCHES "OpenBSD")
    remove_definitions(-D_XOPEN_SOURCE=600)
    add_compile_definitions(_XOPEN_SOURCE=700)
endif()

# Data types, macros and functions related to controlling CPU affinity
# are available on Linux through GNU extensions in libc
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_compile_definitions(_GNU_SOURCE)
endif()

# RLIMIT_MEMLOCK came in BSD, is not specified in POSIX.1,
# and on macOS its availability depends on enabling Darwin extensions
# similarly on DragonFly, enabling BSD extensions is necessary
if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    add_compile_definitions(_DARWIN_C_SOURCE)
endif()
if (CMAKE_SYSTEM_NAME MATCHES "DragonFly")
    add_compile_definitions(_DARWIN_C_SOURCE)
endif()

# alloca is a non-standard interface that is not visible on BSDs when
# POSIX conformance is specified, but not all of them provide a clean way
# to enable it in such cases
if (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
    add_compile_definitions(__BSD_VISIBLE)
endif()
if (CMAKE_SYSTEM_NAME MATCHES "NetBSD")
    add_compile_definitions(_NETBSD_SOURCE)
endif()
if (CMAKE_SYSTEM_NAME MATCHES "OpenBSD")
    add_compile_definitions(_BSD_SOURCE)
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

if (WHISPER_OPENVINO)
    set(TARGET whisper.openvino)

    add_library(${TARGET} OBJECT
        openvino/whisper-openvino-encoder.h
        openvino/whisper-openvino-encoder.cpp
        )

    target_include_directories(${TARGET} PUBLIC
        .
        )

    set_property(TARGET ${TARGET} PROPERTY POSITION_INDEPENDENT_CODE ON)
    set(WHISPER_EXTRA_FLAGS ${WHISPER_EXTRA_FLAGS} -DWHISPER_USE_OPENVINO)

    target_link_libraries(${TARGET} PRIVATE openvino::runtime)
endif()

#
# whisper - this is the main library of the project
#

set(TARGET whisper)

add_library(${TARGET}
    ggml.h
    ggml.c
    ggml-alloc.h
    ggml-alloc.c
    ggml-backend.h
    ggml-backend.c
    ggml-quants.h
    ggml-quants.c
    ${GGML_SOURCES_METAL}
    ${GGML_SOURCES_CUDA}
    ${GGML_SOURCES_OPENCL}
    ${GGML_SOURCES_SYCL}
    ${GGML_HEADERS_SYCL}
    whisper.h
    whisper.cpp
    )

# Set the version numbers
set_target_properties(whisper PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${SOVERSION}
)

include(DefaultTargetOptions)

target_include_directories(${TARGET} PUBLIC
    .
    )

if (WHISPER_COREML)
    target_link_libraries(${TARGET} PRIVATE whisper.coreml)
endif()

if (WHISPER_OPENVINO)
    target_link_libraries(${TARGET} PRIVATE whisper.openvino)
endif()

if (MSVC)
    target_link_libraries(${TARGET} PRIVATE ${WHISPER_EXTRA_LIBS} ${CMAKE_THREAD_LIBS_INIT})

    set(WHISPER_EXTRA_FLAGS ${WHISPER_EXTRA_FLAGS} -D_CRT_SECURE_NO_WARNINGS)
else()
    target_link_libraries(${TARGET} PRIVATE m ${WHISPER_EXTRA_LIBS} ${CMAKE_THREAD_LIBS_INIT})
endif()

if (BUILD_SHARED_LIBS)
    set_target_properties(${TARGET} PROPERTIES POSITION_INDEPENDENT_CODE ON)
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

    if (WHISPER_METAL)
        # TODO: I think this should make ggml-metal.m "see" the ggml-metal.metal file from the "bin" directory
        #       but for some reason it does not work here like it does in llama.cpp
        set_target_properties(${TARGET} PROPERTIES RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/ggml-metal.metal")
    endif()
endif()

if (GGML_SOURCES_CUDA)
    message(STATUS "GGML CUDA sources found, configuring CUDA architecture")
    # Only configure gmml CUDA architectures is not globally set
    if (NOT DEFINED GGML_CUDA_ARCHITECTURES)
        # Not overriden by user, so set defaults
        set(GGML_CUDA_ARCHITECTURES 52 61 70)
    endif()
    message(STATUS "GGML Configuring CUDA architectures ${GGML_CUDA_ARCHITECTURES}")
    set_property(TARGET whisper PROPERTY CUDA_ARCHITECTURES ${GGML_CUDA_ARCHITECTURES})
    set_property(TARGET whisper PROPERTY CUDA_SELECT_NVCC_ARCH_FLAGS "Auto")
endif()

if (EMSCRIPTEN)
    set_target_properties(${TARGET} PROPERTIES COMPILE_FLAGS "-msimd128")
endif()

target_compile_definitions(${TARGET} PUBLIC
    ${WHISPER_EXTRA_FLAGS}
    )

set_target_properties(${TARGET} PROPERTIES PUBLIC_HEADER "ggml.h;whisper.h")

include(GNUInstallDirs)

install(TARGETS ${TARGET}
    LIBRARY  DESTINATION lib
    ARCHIVE  DESTINATION lib/static
    RUNTIME  DESTINATION bin
    RESOURCE DESTINATION bin
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
