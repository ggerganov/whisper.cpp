# stream-stdin
set(TARGET stream-stdin)
add_executable(${TARGET} stream-stdin.cpp)

include(DefaultTargetOptions)

target_link_libraries(${TARGET} PRIVATE common whisper ${CMAKE_THREAD_LIBS_INIT})
