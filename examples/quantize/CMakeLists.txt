set(TARGET whisper-quantize)
add_executable(${TARGET} quantize.cpp)

install(TARGETS ${TARGET} RUNTIME DESTINATION bin)

include(DefaultTargetOptions)

target_link_libraries(${TARGET} PRIVATE common whisper ${CMAKE_THREAD_LIBS_INIT})
