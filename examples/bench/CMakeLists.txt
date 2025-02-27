set(TARGET whisper-bench)
add_executable(${TARGET} bench.cpp)

install(TARGETS ${TARGET} RUNTIME DESTINATION bin)

include(DefaultTargetOptions)

target_link_libraries(${TARGET} PRIVATE whisper ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS ${TARGET} RUNTIME)
