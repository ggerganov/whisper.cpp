if (WHISPER_SUPPORT_SDL2)
    # talk-llama
    set(TARGET talk-llama)

    add_executable(${TARGET} talk-llama.cpp llama.cpp)

    include(DefaultTargetOptions)

    target_link_libraries(${TARGET} PRIVATE common common-sdl whisper ${SDL2_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
endif ()
