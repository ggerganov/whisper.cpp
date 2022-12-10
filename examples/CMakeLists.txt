# dependencies

find_package(Threads REQUIRED)

# third-party

if (WHISPER_SUPPORT_SDL2)
    # SDL2
    find_package(SDL2 REQUIRED)

    string(STRIP "${SDL2_LIBRARIES}" SDL2_LIBRARIES)

    message(STATUS "SDL2_INCLUDE_DIRS = ${SDL2_INCLUDE_DIRS}")
    message(STATUS "SDL2_LIBRARIES = ${SDL2_LIBRARIES}")
endif()

# examples

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

if (EMSCRIPTEN)
    add_subdirectory(whisper.wasm)
    add_subdirectory(stream.wasm)
    add_subdirectory(command.wasm)
    add_subdirectory(talk.wasm)
else()
    add_subdirectory(main)
    add_subdirectory(stream)
    add_subdirectory(command)
    add_subdirectory(bench)
    add_subdirectory(talk)
endif()
