find_package (Threads REQUIRED)

# Allow explicitly specifying glslc executable path from parent CMake
if(DEFINED GGML_VULKAN_GLSLC_EXECUTABLE)
    set(GLSLC_EXECUTABLE ${GGML_VULKAN_GLSLC_EXECUTABLE})
    message(STATUS "Using explicitly provided glslc: ${GLSLC_EXECUTABLE}")
else()
    find_program(GLSLC_EXECUTABLE glslc)
    if(NOT GLSLC_EXECUTABLE)
        message(FATAL_ERROR "glslc not found.")
    endif()
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
