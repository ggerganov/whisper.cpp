set(TARGET whisper.wasm)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/index-tmpl.html        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET}/index.html @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/../helpers.js          ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET}/helpers.js @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/bindings/javascript/whisper.js ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET}/whisper.js  COPYONLY)
