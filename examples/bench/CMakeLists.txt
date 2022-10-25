set(TARGET bench)
add_executable(${TARGET} bench.cpp)
target_link_libraries(${TARGET} PRIVATE whisper ${CMAKE_THREAD_LIBS_INIT})
