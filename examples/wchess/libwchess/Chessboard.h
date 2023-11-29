#pragma once
#include <string>
#include <array>
#include <vector>

class Chessboard {
public:
    Chessboard();
    std::string process(const std::string& t);
    std::string stringifyBoard();
    std::string getRules() const;
    using Move = std::pair<int, int>;
private:
    bool move(const Move& move);

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
            White,
            Black,
        };

        Types type;
        Colors color;
        int pos;
    };

    Piece::Types tokenToType(std::string_view token);
    size_t tokenToPos(std::string_view token);
    using PieceSet = std::array<Piece, 16>;

    PieceSet blackPieces;
    PieceSet whitePieces;
    int m_moveCounter = 0;

    using Board = std::array<Piece*, 64>;
    Board board;

    std::vector<Move> whiteMoves;
    std::vector<Move> blackMoves;

    bool validateMove(const Piece& piece, int pos);
    void getValidMoves(const Piece& piece, std::vector<Move>& moves);
    // just basic validation
    // fixme: missing en passant, castling, promotion, etc.
    bool validatePawnMove(Piece::Colors color, int from_rank, int from_file, int to_rank, int to_file);
    bool validateKnightMove(Piece::Colors color, int from_rank, int from_file, int to_rank, int to_file);
    bool validateBishopMove(Piece::Colors color, int from_rank, int from_file, int to_rank, int to_file);
    bool validateRookMove(Piece::Colors color, int from_rank, int from_file, int to_rank, int to_file);
    bool validateQueenMove(Piece::Colors color, int from_rank, int from_file, int to_rank, int to_file);
    bool validateKingMove(Piece::Colors color, int from_rank, int from_file, int to_rank, int to_file);
};
