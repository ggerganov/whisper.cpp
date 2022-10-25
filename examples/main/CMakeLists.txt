set(TARGET main)
add_executable(${TARGET} main.cpp)
target_link_libraries(${TARGET} PRIVATE whisper ${CMAKE_THREAD_LIBS_INIT})
