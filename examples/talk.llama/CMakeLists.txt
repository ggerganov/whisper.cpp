if (WHISPER_SUPPORT_SDL2)
    # talk.llama
    set(TARGET talk-llama)

    # TODO: this is temporary
    #       need to export ggml symbols for MSVC, but too lazy ..
    add_executable(${TARGET} talk-llama.cpp llama.cpp ../common.cpp ../common-sdl.cpp ../../ggml.c ../../whisper.cpp)

    include(DefaultTargetOptions)

    target_include_directories(${TARGET} PRIVATE ${SDL2_INCLUDE_DIRS} ../../)
    target_link_libraries(${TARGET} PRIVATE ${SDL2_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
endif ()
