set(TARGET whisper-wer)
add_executable(${TARGET} cli.cpp)

include(DefaultTargetOptions)

target_link_libraries(${TARGET} PRIVATE common ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS ${TARGET} RUNTIME)
