if (EMSCRIPTEN)
    #
    # test-whisper-js

    set(TEST_TARGET test-whisper-js)

    add_test(NAME ${TEST_TARGET}
        COMMAND node test-whisper.js --experimental-wasm-threads
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )

    return()
endif()

set(TEST_TARGET test-whisper-cli-tiny)
add_test(NAME ${TEST_TARGET}
    COMMAND $<TARGET_FILE:whisper-cli>
    -m ${PROJECT_SOURCE_DIR}/models/for-tests-ggml-tiny.bin -l fr
    -f ${PROJECT_SOURCE_DIR}/samples/jfk.wav)
set_tests_properties(${TEST_TARGET} PROPERTIES LABELS "tiny;gh")

set(TEST_TARGET test-whisper-cli-tiny.en)
add_test(NAME ${TEST_TARGET}
    COMMAND $<TARGET_FILE:whisper-cli>
    -m ${PROJECT_SOURCE_DIR}/models/for-tests-ggml-tiny.en.bin
    -f ${PROJECT_SOURCE_DIR}/samples/jfk.wav)
set_tests_properties(${TEST_TARGET} PROPERTIES LABELS "tiny;en;gh")

set(TEST_TARGET test-whisper-cli-base)
add_test(NAME ${TEST_TARGET}
    COMMAND $<TARGET_FILE:whisper-cli>
    -m ${PROJECT_SOURCE_DIR}/models/for-tests-ggml-base.bin -l fr
    -f ${PROJECT_SOURCE_DIR}/samples/jfk.wav)
set_tests_properties(${TEST_TARGET} PROPERTIES LABELS "base")

set(TEST_TARGET test-whisper-cli-base.en)
add_test(NAME ${TEST_TARGET}
    COMMAND $<TARGET_FILE:whisper-cli>
    -m ${PROJECT_SOURCE_DIR}/models/for-tests-ggml-base.en.bin
    -f ${PROJECT_SOURCE_DIR}/samples/jfk.wav)
set_tests_properties(${TEST_TARGET} PROPERTIES LABELS "base;en")

set(TEST_TARGET test-whisper-cli-small)
add_test(NAME ${TEST_TARGET}
    COMMAND $<TARGET_FILE:whisper-cli>
    -m ${PROJECT_SOURCE_DIR}/models/for-tests-ggml-small.bin -l fr
    -f ${PROJECT_SOURCE_DIR}/samples/jfk.wav)
set_tests_properties(${TEST_TARGET} PROPERTIES LABELS "small")

set(TEST_TARGET test-whisper-cli-small.en)
add_test(NAME ${TEST_TARGET}
    COMMAND $<TARGET_FILE:whisper-cli>
    -m ${PROJECT_SOURCE_DIR}/models/for-tests-ggml-small.en.bin
    -f ${PROJECT_SOURCE_DIR}/samples/jfk.wav)
set_tests_properties(${TEST_TARGET} PROPERTIES LABELS "small;en")

set(TEST_TARGET test-whisper-cli-medium)
add_test(NAME ${TEST_TARGET}
    COMMAND $<TARGET_FILE:whisper-cli>
    -m ${PROJECT_SOURCE_DIR}/models/for-tests-ggml-medium.bin -l fr
    -f ${PROJECT_SOURCE_DIR}/samples/jfk.wav)
set_tests_properties(${TEST_TARGET} PROPERTIES LABELS "medium")

set(TEST_TARGET test-whisper-cli-medium.en)
add_test(NAME ${TEST_TARGET}
    COMMAND $<TARGET_FILE:whisper-cli>
    -m ${PROJECT_SOURCE_DIR}/models/for-tests-ggml-medium.en.bin
    -f ${PROJECT_SOURCE_DIR}/samples/jfk.wav)
set_tests_properties(${TEST_TARGET} PROPERTIES LABELS "medium;en")

set(TEST_TARGET test-whisper-cli-large)
add_test(NAME ${TEST_TARGET}
    COMMAND $<TARGET_FILE:whisper-cli>
    -m ${PROJECT_SOURCE_DIR}/models/for-tests-ggml-large.bin
    -f ${PROJECT_SOURCE_DIR}/samples/jfk.wav)
set_tests_properties(${TEST_TARGET} PROPERTIES LABELS "large")

if (WHISPER_FFMPEG)
    set(TEST_TARGET test-whisper-cli-tiny-mp3)
    # Check with reviewers: any way to check the output transcription via ctest (diff, ...)?
    add_test(NAME ${TEST_TARGET}
      COMMAND $<TARGET_FILE:whisper-cli>
      -m ${PROJECT_SOURCE_DIR}/models/for-tests-ggml-tiny.en.bin
      -f ${PROJECT_SOURCE_DIR}/samples/jfk.mp3)
    set_tests_properties(${TEST_TARGET} PROPERTIES LABELS "tiny;mp3")
endif()

