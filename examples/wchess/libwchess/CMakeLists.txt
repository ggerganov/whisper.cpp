add_library(libwchess
    WChess.cpp
    WChess.h
    Chessboard.cpp
    Chessboard.h
)

target_link_libraries(libwchess
    PUBLIC
    whisper
    common
)

target_include_directories(libwchess
    PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
)
