set(TARGET server)
add_executable(${TARGET} server.cpp httplib.h json.hpp)

include(DefaultTargetOptions)

target_link_libraries(${TARGET} PRIVATE common whisper ${CMAKE_THREAD_LIBS_INIT})

# Check if the compiler is MinGW
if(MINGW)
    # Link the necessary libraries for SSL and Winsock
    target_link_libraries(${TARGET} PRIVATE -lcrypt32 -lssl -lcrypto -lws2_32)
endif()
