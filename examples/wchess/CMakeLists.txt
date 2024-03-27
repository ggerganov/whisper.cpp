add_subdirectory(libwchess)

if (EMSCRIPTEN)
    add_subdirectory(wchess.wasm)
else()
    add_subdirectory(wchess.cmd)
endif()
