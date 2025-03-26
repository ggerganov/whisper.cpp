cmake_minimum_required(VERSION 3.19)
cmake_policy(SET CMP0114 NEW)

# First find glslc on the host system (not target) to ensure we have control over which one is used
find_program(Vulkan_GLSLC_EXECUTABLE glslc NO_CMAKE_FIND_ROOT_PATH)
if(NOT Vulkan_GLSLC_EXECUTABLE)
    message(WARNING "glslc not found on host system. Will attempt to find via Vulkan package")
endif()

# Find Vulkan package with required components
find_package(Vulkan REQUIRED)
message(STATUS "Using glslc: ${Vulkan_GLSLC_EXECUTABLE}")

function(detect_host_compiler)
    if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
        find_program(HOST_C_COMPILER NAMES cl gcc clang NO_CMAKE_FIND_ROOT_PATH)
        find_program(HOST_CXX_COMPILER NAMES cl g++ clang++ NO_CMAKE_FIND_ROOT_PATH)
    else()
        find_program(HOST_C_COMPILER NAMES gcc clang NO_CMAKE_FIND_ROOT_PATH)
        find_program(HOST_CXX_COMPILER NAMES g++ clang++ NO_CMAKE_FIND_ROOT_PATH)
    endif()
    set(HOST_C_COMPILER "${HOST_C_COMPILER}" PARENT_SCOPE)
    set(HOST_CXX_COMPILER "${HOST_CXX_COMPILER}" PARENT_SCOPE)
endfunction()

if (Vulkan_FOUND)
    message(STATUS "Vulkan found")

    ggml_add_backend_library(ggml-vulkan
                             ggml-vulkan.cpp
                             ../../include/ggml-vulkan.h
                            )
    if (CMAKE_CROSSCOMPILING)
        # Check if user has explicitly set these options
        if(NOT DEFINED GGML_VULKAN_COOPMAT_GLSLC_SUPPORT OR NOT DEFINED GGML_VULKAN_COOPMAT2_GLSLC_SUPPORT)
            # Try to detect target capabilities based on architecture/platform
            message(STATUS "Cross-compiling: attempting to auto-detect target shader capabilities")
            
            # Set defaults based on known platforms
            if(ANDROID)
                # Most modern Android devices support cooperative matrices
                message(STATUS "Android target detected: enabling COOPMAT support by default")
                set(DETECTED_COOPMAT_SUPPORT ON)
                
                # Check Android API level or NDK version for COOPMAT2 support
                # Android NDK r25+ and modern devices generally support this
                if(ANDROID_PLATFORM_LEVEL GREATER_EQUAL 29 OR (DEFINED ANDROID_NDK_REVISION AND ANDROID_NDK_REVISION GREATER_EQUAL 25))
                    message(STATUS "Modern Android target detected: enabling COOPMAT2 support by default")
                    set(DETECTED_COOPMAT2_SUPPORT ON)
                else()
                    set(DETECTED_COOPMAT2_SUPPORT OFF)
                endif()
            elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|ARM64")
                # ARM64 generally supports cooperative matrices on newer GPUs
                message(STATUS "ARM64 target detected: enabling COOPMAT support by default")
                set(DETECTED_COOPMAT_SUPPORT ON)
                set(DETECTED_COOPMAT2_SUPPORT OFF)  # Be conservative with COOPMAT2
            else()
                # For other platforms, default to OFF to be safe
                set(DETECTED_COOPMAT_SUPPORT OFF)
                set(DETECTED_COOPMAT2_SUPPORT OFF)
            endif()
            
            # Set the options based on detection if not already set
            if(NOT DEFINED GGML_VULKAN_COOPMAT_GLSLC_SUPPORT)
                option(GGML_VULKAN_COOPMAT_GLSLC_SUPPORT "Enable coopmat shaders" ${DETECTED_COOPMAT_SUPPORT})
            endif()
            
            if(NOT DEFINED GGML_VULKAN_COOPMAT2_GLSLC_SUPPORT)
                option(GGML_VULKAN_COOPMAT2_GLSLC_SUPPORT "Enable coopmat2 shaders" ${DETECTED_COOPMAT2_SUPPORT})
            endif()
        else()
            # Options already set, use those values
            option(GGML_VULKAN_COOPMAT_GLSLC_SUPPORT "Enable coopmat shaders" ${GGML_VULKAN_COOPMAT_GLSLC_SUPPORT})
            option(GGML_VULKAN_COOPMAT2_GLSLC_SUPPORT "Enable coopmat2 shaders" ${GGML_VULKAN_COOPMAT2_GLSLC_SUPPORT})
        endif()
        
        message(STATUS "Cross-compile COOPMAT support: ${GGML_VULKAN_COOPMAT_GLSLC_SUPPORT}")
        message(STATUS "Cross-compile COOPMAT2 support: ${GGML_VULKAN_COOPMAT2_GLSLC_SUPPORT}")
    else()
        # Compile a test shader to determine whether GL_KHR_cooperative_matrix is supported.
        # If it's not, there will be an error to stderr.
        # If it's supported, set a define to indicate that we should compile those shaders
        execute_process(COMMAND ${Vulkan_GLSLC_EXECUTABLE} -o - -fshader-stage=compute --target-env=vulkan1.3 "${CMAKE_CURRENT_SOURCE_DIR}/vulkan-shaders/test_coopmat_support.comp"
                        OUTPUT_VARIABLE glslc_output
                        ERROR_VARIABLE glslc_error)

        if (${glslc_error} MATCHES ".*extension not supported: GL_KHR_cooperative_matrix.*")
            message(STATUS "GL_KHR_cooperative_matrix not supported by glslc")
            set(GGML_VULKAN_COOPMAT_GLSLC_SUPPORT OFF CACHE INTERNAL "Not enable coopmat shaders")
        else()
            message(STATUS "GL_KHR_cooperative_matrix supported by glslc")
            set(GGML_VULKAN_COOPMAT_GLSLC_SUPPORT ON CACHE INTERNAL "Enable coopmat shaders")
        endif()

        # Compile a test shader to determine whether GL_NV_cooperative_matrix2 is supported.
        # If it's not, there will be an error to stderr.
        # If it's supported, set a define to indicate that we should compile those shaders
        execute_process(COMMAND ${Vulkan_GLSLC_EXECUTABLE} -o - -fshader-stage=compute --target-env=vulkan1.3 "${CMAKE_CURRENT_SOURCE_DIR}/vulkan-shaders/test_coopmat2_support.comp"
                        OUTPUT_VARIABLE glslc_output
                        ERROR_VARIABLE glslc_error)

        if (${glslc_error} MATCHES ".*extension not supported: GL_NV_cooperative_matrix2.*")
            message(STATUS "GL_NV_cooperative_matrix2 not supported by glslc")
            set(GGML_VULKAN_COOPMAT2_GLSLC_SUPPORT OFF CACHE INTERNAL "Not enable coopmat2 shaders")
        else()
            message(STATUS "GL_NV_cooperative_matrix2 supported by glslc")
            set(GGML_VULKAN_COOPMAT2_GLSLC_SUPPORT ON CACHE INTERNAL "Enable coopmat2 shaders")
        endif()
    endif()

    if (GGML_VULKAN_COOPMAT_GLSLC_SUPPORT)
        message(STATUS "GGML_VULKAN_COOPMAT_GLSLC_SUPPORT enabled")
        add_compile_definitions(GGML_VULKAN_COOPMAT_GLSLC_SUPPORT)
    else()
        message(STATUS "GGML_VULKAN_COOPMAT_GLSLC_SUPPORT disabled")
    endif()

    if (GGML_VULKAN_COOPMAT2_GLSLC_SUPPORT)
        message(STATUS "GGML_VULKAN_COOPMAT2_GLSLC_SUPPORT enabled")
        add_compile_definitions(GGML_VULKAN_COOPMAT2_GLSLC_SUPPORT)
    else()
        message(STATUS "GGML_VULKAN_COOPMAT2_GLSLC_SUPPORT disabled")
    endif()

    target_link_libraries(ggml-vulkan PRIVATE Vulkan::Vulkan)
    target_include_directories(ggml-vulkan PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

    # Workaround to the "can't dereference invalidated vector iterator" bug in clang-cl debug build
    # Posssibly relevant: https://stackoverflow.com/questions/74748276/visual-studio-no-displays-the-correct-length-of-stdvector
    if (MSVC AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        add_compile_definitions(_ITERATOR_DEBUG_LEVEL=0)
    endif()

    if (GGML_VULKAN_CHECK_RESULTS)
        add_compile_definitions(GGML_VULKAN_CHECK_RESULTS)
    endif()

    if (GGML_VULKAN_DEBUG)
        add_compile_definitions(GGML_VULKAN_DEBUG)
    endif()

    if (GGML_VULKAN_MEMORY_DEBUG)
        add_compile_definitions(GGML_VULKAN_MEMORY_DEBUG)
    endif()

    if (GGML_VULKAN_SHADER_DEBUG_INFO)
        add_compile_definitions(GGML_VULKAN_SHADER_DEBUG_INFO)
    endif()

    if (GGML_VULKAN_PERF)
        add_compile_definitions(GGML_VULKAN_PERF)
    endif()

    if (GGML_VULKAN_VALIDATE)
        add_compile_definitions(GGML_VULKAN_VALIDATE)
    endif()

    if (GGML_VULKAN_RUN_TESTS)
        add_compile_definitions(GGML_VULKAN_RUN_TESTS)
    endif()

    if (NOT CMAKE_CROSSCOMPILING)
        add_subdirectory(vulkan-shaders)
        if (MSVC)
            foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
                string(TOUPPER ${CONFIG} CONFIG)
                set_target_properties(vulkan-shaders-gen PROPERTIES
                    RUNTIME_OUTPUT_DIRECTORY_${CONFIG} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
            endforeach()
        endif()
    else()
        if (GGML_VULKAN_SHADERS_GEN_TOOLCHAIN)
            set(HOST_CMAKE_TOOLCHAIN_FILE ${GGML_VULKAN_SHADERS_GEN_TOOLCHAIN})
        else()
            detect_host_compiler()
            if (NOT HOST_C_COMPILER OR NOT HOST_CXX_COMPILER)
                message(FATAL_ERROR "Host compiler not found")
            else()
                message(STATUS "Host compiler: ${HOST_C_COMPILER} ${HOST_CXX_COMPILER}")
            endif()
            configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/host-toolchain.cmake.in ${CMAKE_BINARY_DIR}/host-toolchain.cmake @ONLY)
            set(HOST_CMAKE_TOOLCHAIN_FILE ${CMAKE_BINARY_DIR}/host-toolchain.cmake)
        endif()
        message(STATUS "vulkan-shaders-gen toolchain file: ${HOST_CMAKE_TOOLCHAIN_FILE}")

        include(ExternalProject)
        # Native build through ExternalProject_Add
        ExternalProject_Add(
            vulkan-shaders-gen
            SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vulkan-shaders
            CMAKE_ARGS -DCMAKE_TOOLCHAIN_FILE=${HOST_CMAKE_TOOLCHAIN_FILE}
                    -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}
                    -DGGML_VULKAN_COOPMAT_GLSLC_SUPPORT=${GGML_VULKAN_COOPMAT_GLSLC_SUPPORT}
                    -DGGML_VULKAN_COOPMAT2_GLSLC_SUPPORT=${GGML_VULKAN_COOPMAT2_GLSLC_SUPPORT}
                    -DGGML_VULKAN_GLSLC_EXECUTABLE=${Vulkan_GLSLC_EXECUTABLE}
            BUILD_COMMAND ${CMAKE_COMMAND} --build .
            INSTALL_COMMAND ${CMAKE_COMMAND} --install .
            INSTALL_DIR ${CMAKE_BINARY_DIR}
        )
        ExternalProject_Add_StepTargets(vulkan-shaders-gen build install)
    endif()
    set (_ggml_vk_host_suffix $<IF:$<STREQUAL:${CMAKE_HOST_SYSTEM_NAME},Windows>,.exe,>)
    set (_ggml_vk_genshaders_cmd ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/vulkan-shaders-gen${_ggml_vk_host_suffix})
    set (_ggml_vk_header     ${CMAKE_CURRENT_BINARY_DIR}/ggml-vulkan-shaders.hpp)
    set (_ggml_vk_source     ${CMAKE_CURRENT_BINARY_DIR}/ggml-vulkan-shaders.cpp)
    set (_ggml_vk_input_dir  ${CMAKE_CURRENT_SOURCE_DIR}/vulkan-shaders)
    set (_ggml_vk_output_dir ${CMAKE_CURRENT_BINARY_DIR}/vulkan-shaders.spv)

    file(GLOB _ggml_vk_shader_deps "${_ggml_vk_input_dir}/*.comp")
    set (_ggml_vk_shader_deps ${_ggml_vk_shader_deps} vulkan-shaders-gen)

    if (CMAKE_CROSSCOMPILING)
        set(_ggml_vk_shader_deps ${_ggml_vk_shader_deps} vulkan-shaders-gen-build vulkan-shaders-gen-install)
    endif()

    add_custom_command(
        OUTPUT ${_ggml_vk_header}
                ${_ggml_vk_source}

        COMMAND ${_ggml_vk_genshaders_cmd}
            --glslc      ${Vulkan_GLSLC_EXECUTABLE}
            --input-dir  ${_ggml_vk_input_dir}
            --output-dir ${_ggml_vk_output_dir}
            --target-hpp ${_ggml_vk_header}
            --target-cpp ${_ggml_vk_source}
            --no-clean

        DEPENDS ${_ggml_vk_shader_deps}
        COMMENT "Generate vulkan shaders"
    )

    target_sources(ggml-vulkan PRIVATE ${_ggml_vk_source} ${_ggml_vk_header})

else()
    message(WARNING "Vulkan not found")
endif()