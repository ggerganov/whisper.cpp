if (WHISPER_SDL2)
    # stream
    set(TARGET stream_components)
    add_executable(${TARGET} stream_components.cpp)

    include(DefaultTargetOptions)

    target_link_libraries(${TARGET} PRIVATE common common-sdl whisper ${CMAKE_THREAD_LIBS_INIT})
endif ()
