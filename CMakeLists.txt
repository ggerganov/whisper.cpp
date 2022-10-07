cmake_minimum_required (VERSION 3.0)
project(whisper.cpp VERSION 0.1.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS "on")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(WHISPER_STANDALONE ON)
    include(cmake/GitVars.cmake)
    include(cmake/BuildTypes.cmake)
else()
    set(WHISPER_STANDALONE OFF)
endif()

# options

option(WHISPER_ALL_WARNINGS            "whisper: enable all compiler warnings" ON)
option(WHISPER_ALL_WARNINGS_3RD_PARTY  "whisper: enable all compiler warnings in 3rd party libs" OFF)

option(WHISPER_SANITIZE_THREAD         "whisper: enable thread sanitizer" OFF)
option(WHISPER_SANITIZE_ADDRESS        "whisper: enable address sanitizer" OFF)
option(WHISPER_SANITIZE_UNDEFINED      "whisper: enable undefined sanitizer" OFF)

option(WHISPER_BUILD_TESTS             "whisper: build tests"    ${WHISPER_STANDALONE})

option(WHISPER_SUPPORT_SDL2            "whisper: support for libSDL2" OFF)

# sanitizers

if (WHISPER_SANITIZE_THREAD)
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -fsanitize=thread")
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

#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffast-math")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native")

# dependencies

set(CMAKE_C_STANDARD   11)
set(CMAKE_CXX_STANDARD 11)

find_package(Threads REQUIRED)

if (WHISPER_SUPPORT_SDL2)
    # SDL2

    find_package(SDL2 REQUIRED)

    string(STRIP "${SDL2_LIBRARIES}" SDL2_LIBRARIES)

    message(STATUS "SDL2_INCLUDE_DIRS = ${SDL2_INCLUDE_DIRS}")
    message(STATUS "SDL2_LIBRARIES = ${SDL2_LIBRARIES}")
endif()

# main

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo")
endif ()

if (WHISPER_ALL_WARNINGS)
    if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
            -Wall                           \
            -Wextra                         \
            -Wpedantic                      \
            -Wshadow                        \
            -Wcast-qual                     \
            -Wstrict-prototypes             \
            -Wpointer-arith                 \
        ")
    else()
        # todo : windows
    endif()
endif()

# compiler flags

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror=vla")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-math-errno -ffinite-math-only -funsafe-math-optimizations")

message(STATUS "CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")

if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm" OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
    message(STATUS "ARM detected")
else()
    message(STATUS "x86 detected")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mavx -mavx2 -mfma -mf16c")
endif()

# ggml

set(TARGET ggml)

add_library(${TARGET} STATIC
    ggml.c
    )

target_include_directories(${TARGET} PUBLIC
    .
    )

target_link_libraries(${TARGET} PRIVATE m ${CMAKE_THREAD_LIBS_INIT})

if (BUILD_SHARED_LIBS)
    target_link_libraries(${TARGET} PUBLIC
        ${CMAKE_DL_LIBS}
        )

    target_compile_definitions(${TARGET} PUBLIC
        WHISPER_SHARED
        )
endif()

target_compile_definitions(${TARGET} PUBLIC
    ${WHISPER_EXTRA_FLAGS}
    )

# whisper

set(TARGET whisper)

add_library(${TARGET} SHARED
    whisper.cpp
    )

target_include_directories(${TARGET} PUBLIC
    .
    )

target_link_libraries(${TARGET} PRIVATE ggml ${CMALE_THREAD_LIBS_INIT})

if (BUILD_SHARED_LIBS)
    target_link_libraries(${TARGET} PUBLIC
        ${CMAKE_DL_LIBS}
        )

    target_compile_definitions(${TARGET} PUBLIC
        WHISPER_SHARED
        )
endif()

target_compile_definitions(${TARGET} PUBLIC
    ${WHISPER_EXTRA_FLAGS}
    )

install(TARGETS ${TARGET}
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib/static
    )

# main

set(TARGET main)

add_executable(${TARGET} main.cpp)

target_link_libraries(${TARGET} PRIVATE whisper)

# stream

set(TARGET stream)

add_executable(${TARGET} stream.cpp)

target_include_directories(${TARGET} PRIVATE ${SDL2_INCLUDE_DIRS})

target_link_libraries(${TARGET} PRIVATE whisper ${SDL2_LIBRARIES})

# tests

if (WHISPER_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()
