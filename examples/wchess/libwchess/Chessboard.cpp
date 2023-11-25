#include "Chessboard.h"
#include <vector>

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
            else result.push_back('.');
            result.push_back(' ');
        }
        result.push_back('0' + i + 1);
        result.push_back('\n');
    }
    return result;
}

std::string Chessboard::processTranscription(const std::string& t) {
        std::vector<std::string> moves;
        size_t cur = 0;
        size_t last = 0;
        while (cur != std::string::npos) {
            cur = t.find(',', last);
            moves.push_back(t.substr(last, cur));
            last = cur + 1;
        }

        // fixme: lookup depends on grammar
        int count = m_moveCounter;
        std::vector<Move> pendingMoves;
        for (auto& move : moves) {
            fprintf(stdout, "%s: Move '%s%s%s'\n", __func__, "\033[1m", move.c_str(), "\033[0m");
            if (move.empty()) continue;
            auto pieceIndex = 0u;
            for (; pieceIndex < pieceNames.size(); ++pieceIndex) {
                if (std::string::npos != move.find(pieceNames[pieceIndex])) break;
            }
            auto posIndex = 0u;
            for (; posIndex < positions.size(); ++posIndex) {
                if (std::string::npos != move.find(positions[posIndex])) break;
            }
            if (pieceIndex >= pieceNames.size() || posIndex >= positions.size()) continue;

            auto& pieces = count % 2 ? blackPieces : whitePieces;
            auto type = Piece::Types(pieceIndex);
            pieceIndex = 0;
            for (; pieceIndex < pieces.size(); ++pieceIndex) {
                if (pieces[pieceIndex].type == type && checkNext(pieces[pieceIndex], posIndex)) break;
            }

            if (pieceIndex < pieces.size()) {
                pendingMoves.emplace_back(pieces[pieceIndex].pos, posIndex);
            }
            ++count;
        }
        auto result = stringifyMoves(pendingMoves);
        commitMoves(pendingMoves);
        m_moveCounter = count;
        return result;
    }

    bool Chessboard::checkNext(const Piece& piece, int pos, bool kingCheck) {
        if (piece.type == Piece::Taken) return false;
        if (piece.pos == pos) return false;
        int i = piece.pos / 8;
        int j = piece.pos - i * 8;

        int ii = pos / 8;
        int jj = pos - ii * 8;

        if (piece.type == Piece::Pawn) {
            if (piece.color == Piece::White) {
                int direction = piece.color == Piece::White ? 1 : -1;
                if (j == jj) {
                    if (i == ii - direction) return board[pos] == nullptr;
                    if (i == ii - direction * 2) return board[(ii - direction) * 8 + jj] == nullptr && board[pos] == nullptr;
                }
                else if (j + 1 == jj || j - 1 == jj) {
                    if (i == ii - direction) return board[pos] != nullptr && board[pos]->color != piece.color;
                }
            }
            return false;
        }
        if (piece.type == Piece::Knight) {
            int di = std::abs(i - ii);
            int dj = std::abs(j - jj);
            if ((di == 2 && dj == 1) || (di == 1 && dj == 2)) return board[pos] == nullptr || board[pos]->color != piece.color;
            return false;
        }
        if (piece.type == Piece::Bishop) {
            if (i - j == ii - jj) {
                int direction = i < ii ? 1 : -1;
                i += direction;
                j += direction;
                while (i != ii) {
                    if (board[i * 8 + j]) return false;
                    i += direction;
                    j += direction;
                }
                return board[pos] == nullptr || board[pos]->color != piece.color;
            }
            if (i + j == ii + jj) {
                int direction = i < ii ? 1 : -1;
                i += direction;
                j -= direction;
                while (i != ii) {
                    if (board[i * 8 + j]) return false;
                    i += direction;
                    j -= direction;
                }
                return board[pos] == nullptr || board[pos]->color != piece.color;
            }
            return false;
        }
        if (piece.type == Piece::Rook) {
            if (i == ii) {
                int direction = j < jj ? 1 : -1;
                j += direction;
                while (j != jj) {
                    if (board[i * 8 + j]) return false;
                    j += direction;
                }
                return board[pos] == nullptr || board[pos]->color != piece.color;
            }
            if (j == jj) {
                int direction = i < ii ? 1 : -1;
                i += direction;
                while (i != ii) {
                    if (board[i * 8 + j]) return false;
                    i += direction;
                }
                return board[pos] == nullptr || board[pos]->color != piece.color;
            }
            return false;
        }
        if (piece.type == Piece::Queen) {
            if (i - j == ii - jj) {
                int direction = i < ii ? 1 : -1;
                i += direction;
                j += direction;
                while (i != ii) {
                    if (board[i * 8 + j]) return false;
                    i += direction;
                    j += direction;
                }
                return board[pos] == nullptr || board[pos]->color != piece.color;
            }
            if (i + j == ii + jj) {
                int direction = i < ii ? 1 : -1;
                i += direction;
                j -= direction;
                while (i != ii) {
                    if (board[i * 8 + j]) return false;
                    i += direction;
                    j -= direction;
                }
                return board[pos] == nullptr || board[pos]->color != piece.color;
            }
            if (i == ii) {
                int direction = j < jj ? 1 : -1;
                j += direction;
                while (j != jj) {
                    if (board[i * 8 + j]) return false;
                    j += direction;
                }
                return board[pos] == nullptr || board[pos]->color != piece.color;
            }
            if (j == jj) {
                int direction = i < ii ? 1 : -1;
                i += direction;
                while (i != ii) {
                    if (board[i * 8 + j]) return false;
                    i += direction;
                }
                return board[pos] == nullptr || board[pos]->color != piece.color;
            }
            return false;
        }
        if (piece.type == Piece::King) {
            if (std::abs(i - ii) < 2 && std::abs(j - jj) < 2) {
                auto& pieces = piece.color == Piece::White ? whitePieces : blackPieces;
                for (auto& enemyPiece: pieces) {
                    if (!kingCheck && piece.type != Piece::Taken && checkNext(enemyPiece, pos, true)) return false;
                }
                return board[pos] == nullptr || board[pos]->color != piece.color;
            }
        }
        return false;
    }


    std::string Chessboard::stringifyMoves(const std::vector<Move>& pendingMoves) {
        std::string res;
        for (auto& m : pendingMoves) {
            res.append(positions[m.first]);
            res.push_back('-');
            res.append(positions[m.second]);
            res.push_back(' ');
        }
        if (!res.empty()) res.pop_back();
        return res;
    }

    void Chessboard::commitMoves(std::vector<Move>& pendingMoves) {
        for (auto& m : pendingMoves) {
            if (!board[m.first] || (board[m.second] && board[m.first]->type == board[m.second]->type)) continue;
            if (board[m.second]) board[m.second]->type = Piece::Taken;
            board[m.second] = board[m.first];
            board[m.first] = nullptr;
        }
        pendingMoves.clear();
    }