#pragma once
#include <string>
#include <array>
#include <vector>

class Chessboard {
public:
    Chessboard();
    std::string processTranscription(const std::string& t);
    std::string stringifyBoard();
private:
    using Move = std::pair<int, int>;
    std::string stringifyMoves(const std::vector<Move>&);
    void commitMoves(std::vector<Move>&);

    struct Piece {
        enum Types {
            Pawn,
            Knight,
            Bishop,
            Rook,
            Queen,
            King,
            Taken,
        };

        enum Colors {
            Black,
            White
        };

        Types type;
        Colors color;
        int pos;
    };

    using PieceSet = std::array<Piece, 16>;

    PieceSet blackPieces;
    PieceSet whitePieces;
    int m_moveCounter;

    using Board = std::array<Piece*, 64>;
    Board board;

    bool checkNext(const Piece& piece, int pos, bool kingCheck = false);
};
