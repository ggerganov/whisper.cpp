#include "Chessboard.h"
#include <vector>
#include <algorithm>
#include <cassert>
#include <set>

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

constexpr auto INVALID_POS = positions.size();

constexpr int operator ""_P(const char * c, size_t size) {
    if (size < 2) return INVALID_POS;
    int file = c[0] - 'a';
    int rank = c[1] - '1';
    int pos = rank * 8 + file;
    if (pos < 0 || pos >= int(INVALID_POS)) return INVALID_POS;
    return pos;
}

static constexpr std::array<const char*, 6> pieceNames =  {
    "pawn", "knight", "bishop", "rook", "queen", "king",
};

Chessboard::Chessboard()
    : blackPieces {{
        {Piece::Pawn, Piece::Black, "a7"_P },
        {Piece::Pawn, Piece::Black, "b7"_P },
        {Piece::Pawn, Piece::Black, "c7"_P },
        {Piece::Pawn, Piece::Black, "d7"_P },
        {Piece::Pawn, Piece::Black, "e7"_P },
        {Piece::Pawn, Piece::Black, "f7"_P },
        {Piece::Pawn, Piece::Black, "g7"_P },
        {Piece::Pawn, Piece::Black, "h7"_P },
        {Piece::Rook, Piece::Black, "a8"_P },
        {Piece::Knight, Piece::Black, "b8"_P },
        {Piece::Bishop, Piece::Black, "c8"_P },
        {Piece::Queen, Piece::Black, "d8"_P },
        {Piece::King, Piece::Black, "e8"_P },
        {Piece::Bishop, Piece::Black, "f8"_P },
        {Piece::Knight, Piece::Black, "g8"_P },
        {Piece::Rook, Piece::Black, "h8"_P },
    }}
    , whitePieces {{
        {Piece::Pawn, Piece::White, "a2"_P },
        {Piece::Pawn, Piece::White, "b2"_P },
        {Piece::Pawn, Piece::White, "c2"_P },
        {Piece::Pawn, Piece::White, "d2"_P },
        {Piece::Pawn, Piece::White, "e2"_P },
        {Piece::Pawn, Piece::White, "f2"_P },
        {Piece::Pawn, Piece::White, "g2"_P },
        {Piece::Pawn, Piece::White, "h2"_P },
        {Piece::Rook, Piece::White, "a1"_P },
        {Piece::Knight, Piece::White, "b1"_P },
        {Piece::Bishop, Piece::White, "c1"_P },
        {Piece::Queen, Piece::White, "d1"_P },
        {Piece::King, Piece::White, "e1"_P },
        {Piece::Bishop, Piece::White, "f1"_P },
        {Piece::Knight, Piece::White, "g1"_P },
        {Piece::Rook, Piece::White, "h1"_P },
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
    , whiteMoves {
        {"b1"_P, "a3"_P}, {"b1"_P, "c3"_P},
        {"g1"_P, "f3"_P}, {"g1"_P, "h3"_P},
        {"a2"_P, "a3"_P}, {"a2"_P, "a4"_P},
        {"b2"_P, "b3"_P}, {"b2"_P, "b4"_P},
        {"c2"_P, "c3"_P}, {"c2"_P, "c4"_P},
        {"d2"_P, "d3"_P}, {"d2"_P, "d4"_P},
        {"e2"_P, "e3"_P}, {"e2"_P, "e4"_P},
        {"f2"_P, "f3"_P}, {"f2"_P, "f4"_P},
        {"g2"_P, "g3"_P}, {"g2"_P, "g4"_P},
        {"h2"_P, "h3"_P}, {"h2"_P, "h4"_P},
    }
    , blackMoves {
        {"a7"_P, "a5"_P}, {"a7"_P, "a6"_P},
        {"b7"_P, "b5"_P}, {"b7"_P, "b6"_P},
        {"c7"_P, "c5"_P}, {"c7"_P, "c6"_P},
        {"d7"_P, "d5"_P}, {"d7"_P, "d6"_P},
        {"e7"_P, "e5"_P}, {"e7"_P, "e6"_P},
        {"f7"_P, "f5"_P}, {"f7"_P, "f6"_P},
        {"g7"_P, "g5"_P}, {"g7"_P, "g6"_P},
        {"h7"_P, "h5"_P}, {"h7"_P, "h6"_P},
        {"b8"_P, "a6"_P}, {"b8"_P, "c6"_P},
        {"g8"_P, "f6"_P}, {"g8"_P, "h6"_P},
    }

{
    static_assert(pieceNames.size() == Chessboard::Piece::Taken, "Mismatch between piece names and types");
    std::sort(whiteMoves.begin(), whiteMoves.end());
    std::sort(blackMoves.begin(), blackMoves.end());
}

std::string Chessboard::getRules(const std::string& prompt) const {
    // leading space is very important!
    std::string result =
    "\n"
    "# leading space is very important!\n"
    "\n"
    "move ::= prompt \" \" ((piece | frompos) \" \" \"to \"?)? topos\n"
    "\n"
    "prompt ::= \" " + prompt + "\"\n";

    std::set<std::string> pieces;
    std::set<std::string> from_pos;
    std::set<std::string> to_pos;
    auto& allowed_moves =  m_moveCounter % 2 ? blackMoves : whiteMoves;
    for (auto& m : allowed_moves) {
        if (board[m.first]->type != Piece::Taken) pieces.insert(pieceNames[board[m.first]->type]);
        from_pos.insert(positions[m.first]);
        to_pos.insert(positions[m.second]);
    }
    if (!pieces.empty()) {
        result += "piece ::= (";
        for (auto& p : pieces) result += " \"" + p + "\" |";
        result.pop_back();
        result += ")\n\n";
    }
    if (!from_pos.empty()) {
        result += "frompos ::= (";
        for (auto& p : from_pos) result += " \"" + p + "\" |";
        result.pop_back();
        result += ")\n";
    }
    if (!to_pos.empty()) {
        result += "topos ::= (";
        for (auto& p : to_pos) result += " \"" + p + "\" |";
        result.pop_back();
        result += ")\n";
    }

    return result;
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
    return operator ""_P(token.data(), token.size());
}

std::string Chessboard::process(const std::string& command) {
    auto color = Piece::Colors(m_moveCounter % 2);
    fprintf(stdout, "%s: Command to %s: '%s%.*s%s'\n", __func__, (color ? "Black" : "White"), "\033[1m", int(command.size()), command.data(), "\033[0m");
    if (command.empty()) return "";
    auto tokens = split(command, ' ');
    for (auto& t : tokens) fprintf(stdout, "%s: Token %.*s\n", __func__, int(t.size()), t.data());
    auto pos_from = INVALID_POS;
    auto type = Piece::Types::Taken;
    auto pos_to = INVALID_POS;
    if (tokens.size() == 1) {
        type = Piece::Types::Pawn;
        pos_to = tokenToPos(tokens[0]);
    }
    else {
        pos_from = tokenToPos(tokens.front());
        if (pos_from == INVALID_POS) type = tokenToType(tokens.front());
        pos_to = tokenToPos(tokens.back());
    }
    if (pos_to == INVALID_POS) return "";
    if (pos_from == INVALID_POS) {
        if (type == Piece::Types::Taken) return "";
        auto& pieces = color ? blackPieces : whitePieces;
        auto pieceIndex = 0u;
        for (; pieceIndex < pieces.size(); ++pieceIndex) {
            if (pieces[pieceIndex].type == type && validateMove(pieces[pieceIndex], pos_to)) break;
        }
        if (pieceIndex == pieces.size()) return "";
        pos_from = pieces[pieceIndex].pos;
    }
    if (board[pos_from] == nullptr) return "";
    if (board[pos_from]->color != color) return "";

    Move m = {pos_from, pos_to};
    auto& allowed_moves = color ? blackMoves : whiteMoves;
    fprintf(stdout, "%s:allowed size %d :\n", __func__, int(allowed_moves.size()));
    for (auto& m : allowed_moves) fprintf(stdout, " %s %s; ", positions[m.first], positions[m.second]);
    fprintf(stdout, "\n");
    if (!std::binary_search(allowed_moves.begin(), allowed_moves.end(), m)) return "";

    move(m);

    {
        auto it = std::remove_if(allowed_moves.begin(), allowed_moves.end(), [p = m.first] (const Move& m) { return m.first == p; });
        allowed_moves.erase(it, allowed_moves.end());
    }

    std::vector<Piece*> affected = { board[m.second] };
    for (auto& p : whitePieces) {
        if (&p == board[m.second]
            || validateMove(p, m.first)
            || validateMove(p, m.second)
            || std::binary_search(whiteMoves.begin(), whiteMoves.end(), Move(p.pos, m.second))
        ) {
            auto it = std::remove_if(whiteMoves.begin(), whiteMoves.end(), [p = p.pos] (const Move& m) { return m.first == p; });
            whiteMoves.erase(it, whiteMoves.end());
            affected.push_back(&p);
        }
    }

    for (auto& p : blackPieces) {
        if (&p == board[m.second]
            || validateMove(p, m.first)
            || validateMove(p, m.second)
            || std::binary_search(blackMoves.begin(), blackMoves.end(), Move(p.pos, m.second))
        ) {
            auto it = std::remove_if(blackMoves.begin(), blackMoves.end(), [p = p.pos] (const Move& m) { return m.first == p; });
            blackMoves.erase(it, blackMoves.end());
            affected.push_back(&p);
        }
    }
    for (auto& p : affected) getValidMoves(*p, p->color ? blackMoves : whiteMoves);

    std::sort(blackMoves.begin(), blackMoves.end());
    std::sort(whiteMoves.begin(), whiteMoves.end());

    std::string result = positions[m.first];
    result += "-";
    result += positions[m.second];
    ++m_moveCounter;
    fprintf(stdout, "%s: Move '%s%s%s'\n", __func__, "\033[1m", result.data(), "\033[0m");
    return result;
}

void Chessboard::getValidMoves(const Piece& piece, std::vector<Move>& result) {
    std::string cur = positions[piece.pos];
    switch (piece.type) {
        case Piece::Pawn: {
            std::string next = cur;
            piece.color ? --next[1] : ++next[1]; // one down / up
            std::string left = { char(next[0] - 1), next[1]};
            auto pos = tokenToPos(left);
            if (pos != INVALID_POS && board[pos] && board[pos]->color != piece.color) result.emplace_back(piece.pos, pos);
            std::string right = { char(next[0] + 1), next[1]};
            pos = tokenToPos(right);
            if (pos != INVALID_POS && board[pos] && board[pos]->color != piece.color) result.emplace_back(piece.pos, pos);
            pos = tokenToPos(next);
            if (pos != INVALID_POS && !board[pos]) result.emplace_back(piece.pos, pos);
            else break;
            piece.color ? --next[1] : ++next[1]; // one down / up
            pos = tokenToPos(next);
            if (pos != INVALID_POS && !board[pos]) result.emplace_back(piece.pos, pos);
            break;
        }
        case Piece::Knight: {
            std::string next = cur;
            --next[1]; --next[1]; --next[0];
            auto pos = tokenToPos(next);
            if (pos != INVALID_POS && !(board[pos] && board[pos]->color == piece.color)) result.emplace_back(piece.pos, pos);

            next = cur;
            --next[1]; --next[1]; ++next[0];
            pos = tokenToPos(next);
            if (pos != INVALID_POS && !(board[pos] && board[pos]->color == piece.color)) result.emplace_back(piece.pos, pos);

            next = cur;
            ++next[1]; ++next[1]; --next[0];
            pos = tokenToPos(next);
            if (pos != INVALID_POS && !(board[pos] && board[pos]->color == piece.color)) result.emplace_back(piece.pos, pos);

            next = cur;
            ++next[1]; ++next[1]; ++next[0];
            pos = tokenToPos(next);
            if (pos != INVALID_POS && !(board[pos] && board[pos]->color == piece.color)) result.emplace_back(piece.pos, pos);

            next = cur;
            --next[1]; --next[0]; --next[0];
            pos = tokenToPos(next);
            if (pos != INVALID_POS && !(board[pos] && board[pos]->color == piece.color)) result.emplace_back(piece.pos, pos);

            next = cur;
            ++next[1]; --next[0]; --next[0];
            pos = tokenToPos(next);
            if (pos != INVALID_POS && !(board[pos] && board[pos]->color == piece.color)) result.emplace_back(piece.pos, pos);

            next = cur;
            --next[1]; ++next[0]; ++next[0];
            pos = tokenToPos(next);
            if (pos != INVALID_POS && !(board[pos] && board[pos]->color == piece.color)) result.emplace_back(piece.pos, pos);

            next = cur;
            ++next[1]; ++next[0]; ++next[0];
            pos = tokenToPos(next);
            if (pos != INVALID_POS && !(board[pos] && board[pos]->color == piece.color)) result.emplace_back(piece.pos, pos);
            break;
        }
        case Piece::Bishop: {
            std::string next = cur;
            while (true) {
                --next[0]; --next[1];
                auto pos = tokenToPos(next);
                if (pos == INVALID_POS) break;
                else if (board[pos]) {
                    if (board[pos]->color != piece.color) result.emplace_back(piece.pos, pos);
                    break;
                }
                result.emplace_back(piece.pos, pos);
            }
            next = cur;
            while (true) {
                --next[0]; ++next[1];
                auto pos = tokenToPos(next);
                if (pos == INVALID_POS) break;
                else if (board[pos]) {
                    if (board[pos]->color != piece.color) result.emplace_back(piece.pos, pos);
                    break;
                }
                result.emplace_back(piece.pos, pos);
            }
            next = cur;
            while (true) {
                ++next[0]; --next[1];
                auto pos = tokenToPos(next);
                if (pos == INVALID_POS) break;
                else if (board[pos]) {
                    if (board[pos]->color != piece.color) result.emplace_back(piece.pos, pos);
                    break;
                }
                result.emplace_back(piece.pos, pos);
            }
            next = cur;
            while (true) {
                ++next[0]; ++next[1];
                auto pos = tokenToPos(next);
                if (pos == INVALID_POS) break;
                else if (board[pos]) {
                    if (board[pos]->color != piece.color) result.emplace_back(piece.pos, pos);
                    break;
                }
                result.emplace_back(piece.pos, pos);
            }
            break;
        }
        case Piece::Rook: {
            std::string next = cur;
            while (true) {
                --next[0];
                auto pos = tokenToPos(next);
                if (pos == INVALID_POS) break;
                else if (board[pos]) {
                    if (board[pos]->color != piece.color) result.emplace_back(piece.pos, pos);
                    break;
                }
                result.emplace_back(piece.pos, pos);
            }
            next = cur;
            while (true) {
                ++next[0];
                auto pos = tokenToPos(next);
                if (pos == INVALID_POS) break;
                else if (board[pos]) {
                    if (board[pos]->color != piece.color) result.emplace_back(piece.pos, pos);
                    break;
                }
                result.emplace_back(piece.pos, pos);
            }
            next = cur;
            while (true) {
                --next[1];
                auto pos = tokenToPos(next);
                if (pos == INVALID_POS) break;
                else if (board[pos]) {
                    if (board[pos]->color != piece.color) result.emplace_back(piece.pos, pos);
                    break;
                }
                result.emplace_back(piece.pos, pos);
            }
            next = cur;
            while (true) {
                ++next[1];
                auto pos = tokenToPos(next);
                if (pos == INVALID_POS) break;
                else if (board[pos]) {
                    if (board[pos]->color != piece.color) result.emplace_back(piece.pos, pos);
                    break;
                }
                result.emplace_back(piece.pos, pos);
            }
            break;
        }
        case Piece::Queen: {
            std::string next = cur;
            while (true) {
                --next[0]; --next[1];
                auto pos = tokenToPos(next);
                if (pos == INVALID_POS) break;
                else if (board[pos]) {
                    if (board[pos]->color != piece.color) result.emplace_back(piece.pos, pos);
                    break;
                }
                result.emplace_back(piece.pos, pos);
            }
            next = cur;
            while (true) {
                --next[0]; ++next[1];
                auto pos = tokenToPos(next);
                if (pos == INVALID_POS) break;
                else if (board[pos]) {
                    if (board[pos]->color != piece.color) result.emplace_back(piece.pos, pos);
                    break;
                }
                result.emplace_back(piece.pos, pos);
            }
            next = cur;
            while (true) {
                ++next[0]; --next[1];
                auto pos = tokenToPos(next);
                if (pos == INVALID_POS) break;
                else if (board[pos]) {
                    if (board[pos]->color != piece.color) result.emplace_back(piece.pos, pos);
                    break;
                }
                result.emplace_back(piece.pos, pos);
            }
            next = cur;
            while (true) {
                ++next[0]; ++next[1];
                auto pos = tokenToPos(next);
                if (pos == INVALID_POS) break;
                else if (board[pos]) {
                    if (board[pos]->color != piece.color) result.emplace_back(piece.pos, pos);
                    break;
                }
                result.emplace_back(piece.pos, pos);
            }
            next = cur;
            while (true) {
                --next[0];
                auto pos = tokenToPos(next);
                if (pos == INVALID_POS) break;
                else if (board[pos]) {
                    if (board[pos]->color != piece.color) result.emplace_back(piece.pos, pos);
                    break;
                }
                result.emplace_back(piece.pos, pos);
            }
            next = cur;
            while (true) {
                ++next[0];
                auto pos = tokenToPos(next);
                if (pos == INVALID_POS) break;
                else if (board[pos]) {
                    if (board[pos]->color != piece.color) result.emplace_back(piece.pos, pos);
                    break;
                }
                result.emplace_back(piece.pos, pos);
            }
            next = cur;
            while (true) {
                --next[1];
                auto pos = tokenToPos(next);
                if (pos == INVALID_POS) break;
                else if (board[pos]) {
                    if (board[pos]->color != piece.color) result.emplace_back(piece.pos, pos);
                    break;
                }
                result.emplace_back(piece.pos, pos);
            }
            next = cur;
            while (true) {
                ++next[1];
                auto pos = tokenToPos(next);
                if (pos == INVALID_POS) break;
                else if (board[pos]) {
                    if (board[pos]->color != piece.color) result.emplace_back(piece.pos, pos);
                    break;
                }
                result.emplace_back(piece.pos, pos);
            }
            break;
        }
        case Piece::King: {
            std::string next = cur;
            --next[0]; --next[1];
            auto pos = tokenToPos(next);
            if (pos != INVALID_POS && !(board[pos] && board[pos]->color == piece.color)) result.emplace_back(piece.pos, pos);

            next = cur;
            --next[0]; ++next[1];
            pos = tokenToPos(next);
            if (pos != INVALID_POS && !(board[pos] && board[pos]->color == piece.color)) result.emplace_back(piece.pos, pos);

            next = cur;
            ++next[0]; --next[1];
            pos = tokenToPos(next);
            if (pos != INVALID_POS && !(board[pos] && board[pos]->color == piece.color)) result.emplace_back(piece.pos, pos);

            next = cur;
            ++next[0]; ++next[1];
            pos = tokenToPos(next);
            if (pos != INVALID_POS && !(board[pos] && board[pos]->color == piece.color)) result.emplace_back(piece.pos, pos);

            next = cur;
            --next[0];
            pos = tokenToPos(next);
            if (pos != INVALID_POS && !(board[pos] && board[pos]->color == piece.color)) result.emplace_back(piece.pos, pos);

            next = cur;
            ++next[0];
            pos = tokenToPos(next);
            if (pos != INVALID_POS && !(board[pos] && board[pos]->color == piece.color)) result.emplace_back(piece.pos, pos);

            next = cur;
            --next[1];
            pos = tokenToPos(next);
            if (pos != INVALID_POS && !(board[pos] && board[pos]->color == piece.color)) result.emplace_back(piece.pos, pos);

            next = cur;
            ++next[1];
            pos = tokenToPos(next);
            if (pos != INVALID_POS && !(board[pos] && board[pos]->color == piece.color)) result.emplace_back(piece.pos, pos);

            break;
        }
        case Piece::Taken: break;
        default: break;
    }
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