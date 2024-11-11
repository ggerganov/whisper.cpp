set(TARGET server)
add_executable(${TARGET} server.cpp httplib.h thread_pool.h whisper_context.h whisper_params.h)

include(DefaultTargetOptions)

target_link_libraries(${TARGET} PRIVATE common json_cpp whisper ${CMAKE_THREAD_LIBS_INIT})

if (WIN32)
    target_link_libraries(${TARGET} PRIVATE ws2_32)
endif()
