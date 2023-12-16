function(prefix_all NEW_LIST_NAME LIST_NAME PREFIX)
    set(NEW_LIST "")
    foreach(ITEM ${${LIST_NAME}})
        set(NEW_LIST ${NEW_LIST} "${PREFIX}${ITEM}")
    endforeach()
    set(${NEW_LIST_NAME} ${NEW_LIST} PARENT_SCOPE)
endfunction()

if (WHISPER_SDL2)
    # talk-llama
    set(TARGET talk-llama)
    #add_executable(${TARGET} talk-llama.cpp llama.cpp)
    #target_include_directories(${TARGET} PRIVATE ${SDL2_INCLUDE_DIRS})
    #target_link_libraries(${TARGET} PRIVATE common common-sdl whisper ${SDL2_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

    # TODO: this is temporary
    #       need to export ggml symbols for MSVC, but too lazy ..

    # Add ../.. prefix to all the sources
    prefix_all(GGML_SOURCES_METAL_PREF GGML_SOURCES_METAL "../../")
    prefix_all(GGML_SOURCES_CUDA_PREF GGML_SOURCES_CUDA "../../")
    prefix_all(GGML_SOURCES_OPENCL_PREF GGML_SOURCES_OPENCL "../../")

    add_executable(${TARGET}
        talk-llama.cpp
        llama.cpp
        ../common.cpp
        ../common-sdl.cpp
        ../../ggml.c
        ../../ggml-alloc.c
        ../../ggml-backend.c
        ../../ggml-quants.c
        ${GGML_SOURCES_METAL_PREF}
        ${GGML_SOURCES_CUDA_PREF}
        ${GGML_SOURCES_OPENCL_PREF}
        ../../whisper.cpp)

    if(WIN32)
    # It requires Windows 8.1 or later for PrefetchVirtualMemory
    target_compile_definitions(${TARGET} PRIVATE -D_WIN32_WINNT=0x0602)
    endif()

    target_include_directories(${TARGET} PRIVATE ${SDL2_INCLUDE_DIRS} ../../)
    target_link_libraries(${TARGET} PRIVATE ${SDL2_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} ${WHISPER_EXTRA_LIBS})
    target_compile_definitions(${TARGET} PUBLIC ${WHISPER_EXTRA_FLAGS})

    if (WHISPER_COREML)
        target_link_libraries(${TARGET} PRIVATE whisper.coreml)
    endif()

    include(DefaultTargetOptions)
endif ()
