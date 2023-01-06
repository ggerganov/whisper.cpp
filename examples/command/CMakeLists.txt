if (WHISPER_SUPPORT_SDL2)
    # command
    set(TARGET command)
    add_executable(${TARGET} command.cpp)

    include(DefaultTargetOptions)

    target_include_directories(${TARGET} PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(${TARGET} PRIVATE whisper ${SDL2_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
endif ()
