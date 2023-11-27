#include "Chessboard.h"
#include <vector>
#include <algorithm>
#include <cassert>

static constexpr std::array<const char*, 64> positions = {
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
};

static constexpr std::array<const char*, 6> pieceNames =  {
    "pawn", "knight", "bishop", "rook", "queen", "king",
};

Chessboard::Chessboard()
    : blackPieces {{
        {Piece::Pawn, Piece::Black, 48 },
        {Piece::Pawn, Piece::Black, 49 },
        {Piece::Pawn, Piece::Black, 50 },
        {Piece::Pawn, Piece::Black, 51 },
        {Piece::Pawn, Piece::Black, 52 },
        {Piece::Pawn, Piece::Black, 53 },
        {Piece::Pawn, Piece::Black, 54 },
        {Piece::Pawn, Piece::Black, 55 },
        {Piece::Rook, Piece::Black, 56 },
        {Piece::Knight, Piece::Black, 57 },
        {Piece::Bishop, Piece::Black, 58 },
        {Piece::Queen, Piece::Black, 59 },
        {Piece::King, Piece::Black, 60 },
        {Piece::Bishop, Piece::Black, 61 },
        {Piece::Knight, Piece::Black, 62 },
        {Piece::Rook, Piece::Black, 63 },
    }}
    , whitePieces {{
        {Piece::Pawn, Piece::White, 8 },
        {Piece::Pawn, Piece::White, 9 },
        {Piece::Pawn, Piece::White, 10 },
        {Piece::Pawn, Piece::White, 11 },
        {Piece::Pawn, Piece::White, 12 },
        {Piece::Pawn, Piece::White, 13 },
        {Piece::Pawn, Piece::White, 14 },
        {Piece::Pawn, Piece::White, 15 },
        {Piece::Rook, Piece::White, 0 },
        {Piece::Knight, Piece::White, 1 },
        {Piece::Bishop, Piece::White, 2 },
        {Piece::Queen, Piece::White, 3 },
        {Piece::King, Piece::White, 4 },
        {Piece::Bishop, Piece::White, 5 },
        {Piece::Knight, Piece::White, 6 },
        {Piece::Rook, Piece::White, 7 },
    }}
    , board {{
        &whitePieces[ 8], &whitePieces[ 9], &whitePieces[10], &whitePieces[11], &whitePieces[12], &whitePieces[13], &whitePieces[14], &whitePieces[15],
        &whitePieces[ 0], &whitePieces[ 1], &whitePieces[ 2], &whitePieces[ 3], &whitePieces[ 4], &whitePieces[ 5], &whitePieces[ 6], &whitePieces[ 7],
        nullptr,          nullptr,          nullptr,          nullptr,          nullptr,          nullptr,          nullptr,          nullptr,
        nullptr,          nullptr,          nullptr,          nullptr,          nullptr,          nullptr,          nullptr,          nullptr,
        nullptr,          nullptr,          nullptr,          nullptr,          nullptr,          nullptr,          nullptr,          nullptr,
        nullptr,          nullptr,          nullptr,          nullptr,          nullptr,          nullptr,          nullptr,          nullptr,
        &blackPieces[ 0], &blackPieces[ 1], &blackPieces[ 2], &blackPieces[ 3], &blackPieces[ 4], &blackPieces[ 5], &blackPieces[ 6], &blackPieces[ 7],
        &blackPieces[ 8], &blackPieces[ 9], &blackPieces[10], &blackPieces[11], &blackPieces[12], &blackPieces[13], &blackPieces[14], &blackPieces[15],
    }}
{
    static_assert(pieceNames.size() == Chessboard::Piece::Taken, "Mismatch between piece names and types");
}

std::string Chessboard::stringifyBoard() {
    static constexpr std::array<char, 6> blackShort =  {
        'p', 'n', 'b', 'r', 'q', 'k',
    };
    static constexpr std::array<char, 6> whiteShort =  {
        'P', 'N', 'B', 'R', 'Q', 'K',
    };

    std::string result;
    result.reserve(16 + 2 * 64 + 16);
    for (char rank = 'a'; rank <= 'h'; ++rank) {
        result.push_back(rank);
        result.push_back(' ');
    }
    result.back() = '\n';
    for (int i = 7; i >= 0; --i) {
        for (int j = 0; j < 8; ++j) {
            if (auto p = board[i * 8 + j]; p) result.push_back(p->color == Piece::White ? whiteShort[p->type] : blackShort[p->type]);
            else result.push_back((i + j) % 2 ? '.' : '*');
            result.push_back(' ');
        }
        result.push_back('0' + i + 1);
        result.push_back('\n');
    }
return result;
}

std::vector<std::string_view> split(std::string_view str, char del) {
    std::vector<std::string_view> res;
    size_t cur = 0;
    size_t last = 0;
    while (cur != std::string::npos) {
        if (str[last] == ' ') { // trim white
            ++last;
            continue;
        }
        cur = str.find(del, last);
        size_t len = cur == std::string::npos ? str.size() - last : cur - last;
        res.emplace_back(str.data() + last, len);
        last = cur + 1;
    }
    return res;
}

Chessboard::Piece::Types Chessboard::tokenToType(std::string_view token) {
    auto it = std::find(pieceNames.begin(), pieceNames.end(), token);
    return it != pieceNames.end() ? Piece::Types(it - pieceNames.begin()) : Piece::Taken;
}

size_t Chessboard::tokenToPos(std::string_view token) {
    if (token.size() < 2) return positions.size();
    int file = token[0] - 'a';
    int rank = token[1] - '1';
    int pos = rank * 8 + file;
    if (pos < 0 || pos >= int(positions.size())) return positions.size();
    return pos;
}

std::string Chessboard::process(const std::string& transcription) {
    auto commands = split(transcription, ',');

    // fixme: lookup depends on grammar
    int count = m_moveCounter;
    std::vector<Move> moves;
    std::string result;
    result.reserve(commands.size() * 6);
    for (auto& command : commands) {

        fprintf(stdout, "%s: Command '%s%.*s%s'\n", __func__, "\033[1m", int(command.size()), command.data(), "\033[0m");
        if (command.empty()) continue;
        auto tokens = split(command, ' ');
        Piece::Types type = Piece::Types::Taken;
        size_t pos = positions.size();
        if (tokens.size() == 1) {
            type = Piece::Types::Pawn;
            pos = tokenToPos(tokens[0]);
        }
        else if (tokens.size() == 3) {
            type = tokenToType(tokens[0]);
            assert(tokens[1] == "to");
            pos = tokenToPos(tokens[2]);
        }
        if (type == Piece::Types::Taken || pos == positions.size()) continue;

        auto& pieces = count % 2 ? blackPieces : whitePieces;
        auto pieceIndex = 0u;
        for (; pieceIndex < pieces.size(); ++pieceIndex) {
            if (pieces[pieceIndex].type == type && validateMove(pieces[pieceIndex], pos)) break;
        }
        Move m = {pieces[pieceIndex].pos, pos};
        if (pieceIndex < pieces.size() && move({m})) {
            result.append(positions[m.first]);
            result.push_back('-');
            result.append(positions[m.second]);
            result.push_back(' ');
            ++count;
        }
    }
    if (!result.empty()) result.pop_back();
    m_moveCounter = count;
    fprintf(stdout, "%s: Moves '%s%s%s'\n", __func__, "\033[1m", result.data(), "\033[0m");
    return result;
}

bool Chessboard::validatePawnMove(Piece::Colors color, int from_rank, int from_file, int to_rank, int to_file) {
    int direction = color == Piece::White ? 1 : -1;
    if (from_file == to_file) {
        if (from_rank == to_rank - direction) return board[to_rank * 8 + to_file] == nullptr;
        if (from_rank == to_rank - direction * 2) return board[(to_rank - direction) * 8 + to_file] == nullptr && board[to_rank * 8 + to_file] == nullptr;
    }
    else if (from_file + 1 == to_file || from_file - 1 == to_file) {
        if (from_rank == to_rank - direction) return board[to_rank * 8 + to_file] != nullptr && board[to_rank * 8 + to_file]->color != color;
    }
    return false;
}

bool Chessboard::validateKnightMove(Piece::Colors color, int from_rank, int from_file, int to_rank, int to_file) {
    int dr = std::abs(from_rank - to_rank);
    int df = std::abs(from_file - to_file);
    if ((dr == 2 && df == 1) || (dr == 1 && df == 2)) return board[to_rank * 8 + to_file] == nullptr || board[to_rank * 8 + to_file]->color != color;
    return false;
}

bool Chessboard::validateBishopMove(Piece::Colors color, int from_rank, int from_file, int to_rank, int to_file) {
    if (from_rank - from_file == to_rank - to_file) {
        int direction = from_rank < to_rank ? 1 : -1;
        from_rank += direction;
        from_file += direction;
        while (from_rank != to_rank) {
            if (board[from_rank * 8 + from_file]) return false;
            from_rank += direction;
            from_file += direction;
        }
        return board[to_rank * 8 + to_file] == nullptr || board[to_rank * 8 + to_file]->color != color;
    }
    if (from_rank + from_file == to_rank + to_file) {
        int direction = from_rank < to_rank ? 1 : -1;
        from_rank += direction;
        from_file -= direction;
        while (from_rank != to_rank) {
            if (board[from_rank * 8 + from_file]) return false;
            from_rank += direction;
            from_file -= direction;
        }
        return board[to_rank * 8 + to_file] == nullptr || board[to_rank * 8 + to_file]->color != color;
    }
    return false;
}

bool Chessboard::validateRookMove(Piece::Colors color, int from_rank, int from_file, int to_rank, int to_file) {
    if (from_rank == to_rank) {
        int direction = from_file < to_file ? 1 : -1;
        from_file += direction;
        while (from_file != to_file) {
            if (board[from_rank * 8 + from_file]) return false;
            from_file += direction;
        }
        return board[to_rank * 8 + to_file] == nullptr || board[to_rank * 8 + to_file]->color != color;
    }
    if (from_file == to_file) {
        int direction = from_rank < to_rank ? 1 : -1;
        from_rank += direction;
        while (from_rank != to_rank) {
            if (board[from_rank * 8 + from_file]) return false;
            from_rank += direction;
        }
        return board[to_rank * 8 + to_file] == nullptr || board[to_rank * 8 + to_file]->color != color;
    }
    return false;
}

bool Chessboard::validateQueenMove(Piece::Colors color, int from_rank, int from_file, int to_rank, int to_file) {
    if (validateBishopMove(color, from_rank, from_file, to_rank, to_file)) return true;
    return validateRookMove(color, from_rank, from_file, to_rank, to_file);
}

bool Chessboard::validateKingMove(Piece::Colors color, int from_rank, int from_file, int to_rank, int to_file) {
    if (std::abs(from_rank - to_rank) < 2 && std::abs(from_file - to_file) < 2) {
        return board[to_rank * 8 + to_file] == nullptr || board[to_rank * 8 + to_file]->color != color;
    }
    return false;
}

bool Chessboard::validateMove(const Piece& piece, int pos) {
    if (piece.type == Piece::Taken) return false;
    if (piece.pos == pos) return false;
    int i = piece.pos / 8;
    int j = piece.pos - i * 8;

    int ii = pos / 8;
    int jj = pos - ii * 8;

    switch (piece.type) {
        case Piece::Pawn: return validatePawnMove(piece.color, i, j, ii, jj);
        case Piece::Knight: return validateKnightMove(piece.color, i, j, ii, jj);
        case Piece::Bishop: return validateBishopMove(piece.color, i, j, ii, jj);
        case Piece::Rook: return validateRookMove(piece.color, i, j, ii, jj);
        case Piece::Queen: return validateQueenMove(piece.color, i, j, ii, jj);
        case Piece::King: return validateKingMove(piece.color, i, j, ii, jj);
        default: break;
    }
    return false;
}

bool Chessboard::move(const Move& m) {
    if (!board[m.first] || (board[m.second] && board[m.first]->color == board[m.second]->color)) return false;
    if (board[m.second]) board[m.second]->type = Piece::Taken;
    board[m.second] = board[m.first];
    board[m.first] = nullptr;
    board[m.second]->pos = m.second;
    return true;
}