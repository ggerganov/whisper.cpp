if (WHISPER_SDL2)
    # talk-llama
    set(TARGET talk-llama)
    add_executable(${TARGET} talk-llama.cpp llama.cpp)
    target_include_directories(${TARGET} PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(${TARGET} PRIVATE common common-sdl whisper ${SDL2_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

    if(WIN32)
        # It requires Windows 8.1 or later for PrefetchVirtualMemory
        target_compile_definitions(${TARGET} PRIVATE -D_WIN32_WINNT=0x0602)
    endif()

    include(DefaultTargetOptions)
endif ()
