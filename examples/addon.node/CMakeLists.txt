set(TARGET whisper-addon)

# Base settings
#==================================================================
# env var supported by cmake-js
add_definitions(-DNAPI_VERSION=4)
include_directories(${CMAKE_JS_INC})
#==================================================================

add_library(${TARGET} SHARED ${CMAKE_JS_SRC} addon.cpp)
set_target_properties(${TARGET} PROPERTIES PREFIX "" SUFFIX ".node")

include(DefaultTargetOptions)

# Include N-API wrappers
#==================================================================
execute_process(COMMAND node -p "require('node-addon-api').include"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE NODE_ADDON_API_DIR
        )
string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
target_include_directories(${TARGET} PRIVATE ${NODE_ADDON_API_DIR})
#==================================================================

target_link_libraries(${TARGET} ${CMAKE_JS_LIB} whisper ${CMAKE_THREAD_LIBS_INIT})
