if (WHISPER_SDL2)
    # talk-llama
    set(TARGET talk-llama)
    #add_executable(${TARGET} talk-llama.cpp llama.cpp)
    #target_include_directories(${TARGET} PRIVATE ${SDL2_INCLUDE_DIRS})
    #target_link_libraries(${TARGET} PRIVATE common common-sdl whisper ${SDL2_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

    # TODO: this is temporary
    #       need to export ggml symbols for MSVC, but too lazy ..
    add_executable(${TARGET} talk-llama.cpp llama.cpp ../common.cpp ../common-sdl.cpp ../../ggml.c ../../whisper.cpp)

    target_include_directories(${TARGET} PRIVATE ${SDL2_INCLUDE_DIRS} ../../)
    target_link_libraries(${TARGET} PRIVATE ${SDL2_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

    include(DefaultTargetOptions)
endif ()
