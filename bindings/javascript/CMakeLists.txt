set(TARGET libwhisper)

add_executable(${TARGET}
    emscripten.cpp
    )

target_link_libraries(${TARGET} PRIVATE
    whisper
    )

unset(EXTRA_FLAGS)
if (WHISPER_WASM_SINGLE_FILE)
    set(EXTRA_FLAGS "-s SINGLE_FILE=1")
    message(STATUS "Embedding WASM inside whisper.js")

    add_custom_command(
        TARGET libwhisper POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_BINARY_DIR}/bin/libwhisper.js
        ${CMAKE_CURRENT_SOURCE_DIR}/whisper.js
        )
endif()

#-s TOTAL_MEMORY=536870912 \
set_target_properties(${TARGET} PROPERTIES LINK_FLAGS " \
    --bind \
    -s MODULARIZE=1 \
    -s ASSERTIONS=1 \
    -s USE_PTHREADS=1 \
    -s PTHREAD_POOL_SIZE=9 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s FORCE_FILESYSTEM=1 \
    -s EXPORT_NAME=\"'whisper_factory'\" \
    ${EXTRA_FLAGS} \
    ")
