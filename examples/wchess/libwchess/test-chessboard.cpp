#include "Chessboard.h"

#define ASSERT(x) \
    do { \
        if (!(x)) { \
            fprintf(stderr, "ASSERT: %s:%d: %s\n", __FILE__, __LINE__, #x); \
            fflush(stderr); \
            exit(1); \
        } \
    } while (0)


int main() {

    {
        // pawns
        Chessboard chess;

        ASSERT(chess.process("pawn to d4, e5, e3, pawn to d5") == "d2-d4 e7-e5 e2-e3 d7-d5");
        ASSERT(chess.process("pawn to d4") == ""); // wrong
        ASSERT(chess.process("pawn to c5") == ""); // wrong
        ASSERT(chess.process("pawn to d5") == ""); // wrong
        ASSERT(chess.process("pawn to d3") == ""); // wrong
        ASSERT(chess.process("pawn to f5") == ""); // wrong, white's turn
        ASSERT(chess.process("h4") == "h2-h4");
        ASSERT(chess.process("d4") == "e5-d4");
        ASSERT(chess.process("e4") == "e3-e4");
        ASSERT(chess.process("d4") == ""); // wrong
        ASSERT(chess.process("e4") == "d5-e4");
    }

    {
        // rook
        Chessboard chess;

        ASSERT(chess.process("rook to a3") == ""); // wrong
        ASSERT(chess.process("a4, h5, rook to a3, rook to h6") == "a2-a4 h7-h5 a1-a3 h8-h6");
        ASSERT(chess.process("rook to d3, rook to e6") == "a3-d3 h6-e6");
        ASSERT(chess.process("rook to d4, rook to e5") == "d3-d4 e6-e5");
        ASSERT(chess.process("rook to a4") == ""); // wrong
        ASSERT(chess.process("rook to d8") == ""); // wrong
        ASSERT(chess.process("rook to d3") == "d4-d3");
        ASSERT(chess.process("rook to e2") == "e5-e2");
    }

    {
        // knight
        Chessboard chess;

        ASSERT(chess.process("knight to c3, knight to c6") == "b1-c3 b8-c6");
        ASSERT(chess.process("knight to c3") == ""); // wrong
        ASSERT(chess.process("knight to a2") == ""); // wrong
        ASSERT(chess.process("knight to b4") == ""); // wrong, white's turn
        ASSERT(chess.process("knight to b5") == "c3-b5");
        ASSERT(chess.process("knight to a5") == "c6-a5");
        ASSERT(chess.process("knight to c7") == "b5-c7");
    }

    {
        // bishop
        Chessboard chess;

        ASSERT(chess.process("b3, b6, bishop to b2, bishop to b7") == "b2-b3 b7-b6 c1-b2 c8-b7");
        ASSERT(chess.process("bishop to a1") == ""); // wrong
        ASSERT(chess.process("bishop to h8") == ""); // wrong
        ASSERT(chess.process("bishop to a6") == ""); // wrong, white's turn
        ASSERT(chess.process("bishop to g7") == "b2-g7");
    }

    {
        // queen
        Chessboard chess;
        ASSERT(chess.process("queen to d8") == ""); // wrong
        ASSERT(chess.process("queen to f1") == ""); // wrong
        ASSERT(chess.process("queen to h5") == ""); // wrong
        ASSERT(chess.process("e3, d5, queen to h5, queen to d6") == "e2-e3 d7-d5 d1-h5 d8-d6");
        ASSERT(chess.process("queen to c5") == ""); // wrong, white's turn
        ASSERT(chess.process("queen to f7") == "h5-f7");
    }

    {
        // king
        Chessboard chess;
        ASSERT(chess.process("d3, d6, king to d2, king to d7, king to c3, king to c6, king to c4") == "d2-d3 d7-d6 e1-d2 e8-d7 d2-c3 d7-c6 c3-c4");
        ASSERT(chess.process("bishop to e6") == "c8-e6");
        ASSERT(chess.process("king to b3") == "c4-b3"); // !! check check not implemented
    }
}