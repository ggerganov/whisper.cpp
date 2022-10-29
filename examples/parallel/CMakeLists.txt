set(TARGET parallel)
add_executable(${TARGET} parallel.cpp)
target_link_libraries(${TARGET} PRIVATE whisper ${CMAKE_THREAD_LIBS_INIT})
