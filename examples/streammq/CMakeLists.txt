if (WHISPER_SUPPORT_SDL2)
    # stream
    set(TARGET stream)
    add_executable(${TARGET} stream.cpp)

    find_package(cppzmq)
    include(DefaultTargetOptions)

    target_link_libraries(${TARGET} PRIVATE common cppzmq common-sdl whisper ${CMAKE_THREAD_LIBS_INIT})
endif ()


