# dependencies

find_package(Threads REQUIRED)

# third-party

if (WHISPER_SDL2)
    # SDL2
    find_package(SDL2 REQUIRED)

    string(STRIP "${SDL2_LIBRARIES}" SDL2_LIBRARIES)

    message(STATUS "SDL2_INCLUDE_DIRS = ${SDL2_INCLUDE_DIRS}")
    message(STATUS "SDL2_LIBRARIES = ${SDL2_LIBRARIES}")
endif()

if (WHISPER_GRPC)
    # GRPC
    include(GRPC.cmake)
endif()

if (WHISPER_CLBLAST)
    find_package(CLBlast REQUIRED)
endif()

# common

set(TARGET common)

add_library(${TARGET} STATIC
    common.h
    common.cpp
    common-ggml.h
    common-ggml.cpp
    grammar-parser.cpp
    )

include(DefaultTargetOptions)

target_link_libraries(${TARGET} PRIVATE whisper)

set_target_properties(${TARGET} PROPERTIES POSITION_INDEPENDENT_CODE ON)

if (WHISPER_SDL2)
    # common-sdl

    set(TARGET common-sdl)

    add_library(${TARGET} STATIC
        common-sdl.h
        common-sdl.cpp
        )

    include(DefaultTargetOptions)

    target_include_directories(${TARGET} PUBLIC ${SDL2_INCLUDE_DIRS})
    target_link_libraries(${TARGET} PRIVATE ${SDL2_LIBRARIES})

    set_target_properties(${TARGET} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

if (WHISPER_GRPC)
    # common-grpc

    set(TARGET common-grpc)

    #
    # GENERATE PROTOBUF CODE
    #

    # Proto file
    get_filename_component(sg_proto "transcription.proto" ABSOLUTE)
    get_filename_component(sg_proto_path "${sg_proto}" PATH)

    # Generated sources
    set(sg_proto_srcs "transcription.pb.cc")
    set(sg_proto_hdrs "transcription.pb.h")
    set(sg_grpc_srcs "transcription.grpc.pb.cc")
    set(sg_grpc_hdrs "transcription.grpc.pb.h")
    add_custom_command(
        OUTPUT "${sg_proto_srcs}" "${sg_proto_hdrs}" "${sg_grpc_srcs}" "${sg_grpc_hdrs}"
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
            --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
            -I "${sg_proto_path}"
            --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
            "${sg_proto}"
        DEPENDS "${sg_proto}")    

    #
    # BUILD common GRPC audio library (with our protobuf)
    #

    # Common deps
    set(sg_common_srcs "common-grpc.cpp")
    set(sg_common_hdrs "common-grpc.h")    
    
    # Include generated *.pb.h files
    include_directories("${CMAKE_CURRENT_BINARY_DIR}")

    add_library(${TARGET} 
        ${sg_grpc_srcs}
        ${sg_grpc_hdrs}
        ${sg_proto_srcs}
        ${sg_proto_hdrs}
        ${sg_common_srcs}
        ${sg_common_hdrs}  
    )
    target_link_libraries(${TARGET} 
        ${_REFLECTION}
        ${_GRPC_GRPCPP}
        ${_PROTOBUF_LIBPROTOBUF}
    )    

    include(DefaultTargetOptions)

    set_target_properties(${TARGET} PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

# examples

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

if (EMSCRIPTEN)
    add_subdirectory(whisper.wasm)
    add_subdirectory(stream.wasm)
    add_subdirectory(command.wasm)
    add_subdirectory(talk.wasm)
    add_subdirectory(bench.wasm)
elseif(CMAKE_JS_VERSION)
    add_subdirectory(addon.node)
else()
    add_subdirectory(main)
    add_subdirectory(stream)
    add_subdirectory(stream.grpc)
    add_subdirectory(server)
    add_subdirectory(command)
    add_subdirectory(bench)
    add_subdirectory(quantize)
    add_subdirectory(talk)
    add_subdirectory(talk-llama)
    add_subdirectory(lsp)
endif()

add_subdirectory(wchess)
