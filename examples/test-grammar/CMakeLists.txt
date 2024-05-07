add_executable(test-grammar
    test-grammar.cpp
    )

include(DefaultTargetOptions)

target_link_libraries(test-grammar PRIVATE
    whisper
    common
    )