set(CMAKE_CXX_STANDARD 11)

add_subdirectory(libwchess)

if (EMSCRIPTEN)
    add_subdirectory(wchess.wasm)
else()
    add_subdirectory(wchess.cmd)
endif()
