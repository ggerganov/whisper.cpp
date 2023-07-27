if (WHISPER_SDL2)
    # talk
    set(TARGET talk)
    add_executable(${TARGET} talk.cpp gpt-2.cpp)
    target_link_libraries(${TARGET} PRIVATE common common-sdl whisper ${CMAKE_THREAD_LIBS_INIT})

    include(DefaultTargetOptions)
endif ()
