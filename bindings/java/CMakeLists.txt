cmake_minimum_required(VERSION 3.10)

project(whisper_java VERSION 1.4.2)

# Set the target name and source file/s
set(TARGET_NAME whisper_java)
set(SOURCES src/main/cpp/whisper_java.cpp)

# include <whisper.h>
include_directories(../../)

# Set the output directory for the DLL/shared library based on the platform as required by JNA
if(WIN32)
    set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated/resources/main/win32-x86-64)
elseif(UNIX AND NOT APPLE)
    set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated/resources/main/linux-x86-64)
elseif(APPLE)
    set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated/resources/main/macos-x86-64)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_DIR})

# Create the whisper_java library
add_library(${TARGET_NAME} SHARED ${SOURCES})

# Link against ../../build/Release/whisper.dll (or so/dynlib)
target_link_directories(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/../../../build/${CMAKE_BUILD_TYPE})
target_link_libraries(${TARGET_NAME} PRIVATE whisper)

# Set the appropriate compiler flags for Windows, Linux, and macOS
if(WIN32)
    target_compile_options(${TARGET_NAME} PRIVATE /W4 /D_CRT_SECURE_NO_WARNINGS)
elseif(UNIX AND NOT APPLE)
    target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra)
elseif(APPLE)
    target_compile_options(${TARGET_NAME} PRIVATE -Wall -Wextra)
endif()

target_compile_definitions(${TARGET_NAME} PRIVATE WHISPER_SHARED)
# add_definitions(-DWHISPER_SHARED)

# Force CMake to save the libs to build/generated/resources/main/${os}-${arch} as required by JNA
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set_target_properties(${TARGET_NAME} PROPERTIES
                          RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${OUTPUT_DIR}
                          LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${OUTPUT_DIR}
                          ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${OUTPUT_DIR})
endforeach(OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)
