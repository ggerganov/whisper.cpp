find_package (Threads REQUIRED)

# Use glslc from the parent CMake context or find it on the host system
if(DEFINED Vulkan_GLSLC_EXECUTABLE)
    set(GLSLC_EXECUTABLE ${Vulkan_GLSLC_EXECUTABLE})
    message(STATUS "Using glslc from parent CMake: ${GLSLC_EXECUTABLE}")
    # Also define this at compile time to set the default value in the shader generator
    add_compile_definitions(VULKAN_GLSLC_EXECUTABLE="${GLSLC_EXECUTABLE}")
else()
    # Find glslc on the host system, not the target
    find_program(GLSLC_EXECUTABLE glslc NO_CMAKE_FIND_ROOT_PATH)
    if(NOT GLSLC_EXECUTABLE)
        message(FATAL_ERROR "glslc not found. Please set Vulkan_GLSLC_EXECUTABLE in parent CMake.")
    endif()
    message(STATUS "Found host glslc: ${GLSLC_EXECUTABLE}")
endif()

option(GGML_VULKAN_COOPMAT_GLSLC_SUPPORT "Enable coopmat shaders" OFF)
option(GGML_VULKAN_COOPMAT2_GLSLC_SUPPORT "Enable coopmat2 shaders" OFF)

message(STATUS "GGML_VULKAN_COOPMAT_GLSLC_SUPPORT: ${GGML_VULKAN_COOPMAT_GLSLC_SUPPORT}")
message(STATUS "GGML_VULKAN_COOPMAT2_GLSLC_SUPPORT: ${GGML_VULKAN_COOPMAT2_GLSLC_SUPPORT}")

set(TARGET vulkan-shaders-gen)
add_executable(${TARGET} vulkan-shaders-gen.cpp)
if (GGML_VULKAN_COOPMAT_GLSLC_SUPPORT)
    target_compile_definitions(vulkan-shaders-gen PRIVATE GGML_VULKAN_COOPMAT_GLSLC_SUPPORT)
endif()

if (GGML_VULKAN_COOPMAT2_GLSLC_SUPPORT)
    target_compile_definitions(vulkan-shaders-gen PRIVATE GGML_VULKAN_COOPMAT2_GLSLC_SUPPORT)
endif()

install(TARGETS ${TARGET} RUNTIME)
target_compile_features(${TARGET} PRIVATE cxx_std_17)
target_link_libraries(vulkan-shaders-gen PUBLIC Threads::Threads)
