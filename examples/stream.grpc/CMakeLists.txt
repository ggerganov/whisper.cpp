if (WHISPER_GRPC)
    # stream
    set(TARGET stream.grpc)
    add_executable(${TARGET} stream.cpp)

    include(DefaultTargetOptions)

    target_link_libraries(${TARGET} PRIVATE 
      common 
      common-grpc 
      whisper 
      ${CMAKE_THREAD_LIBS_INIT}
      ${_REFLECTION}
      ${_GRPC_GRPCPP}
      ${_PROTOBUF_LIBPROTOBUF}       
    )
endif ()