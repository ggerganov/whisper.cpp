# dependencies

find_package(Threads REQUIRED)

# third-party

#add_subdirectory(third-party)

# examples

if (EMSCRIPTEN)
    add_subdirectory(whisper.wasm)
else()
endif()
