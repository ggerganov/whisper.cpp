#include "ggml.h"
#include "common.h"
#include "whisper.h"
#include "grammar-parser.h"

#include <emscripten.h>
#include <emscripten/bind.h>

#include <atomic>
#include <cmath>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <regex>

constexpr int N_THREAD = 8;

std::vector<struct whisper_context *> g_contexts(4, nullptr);

std::mutex  g_mutex;
std::thread g_worker;

std::atomic<bool> g_running(false);

std::string g_status        = "";
std::string g_status_forced = "";
std::string g_transcribed   = "";

std::vector<float> g_pcmf32;

void command_set_status(const std::string & status) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_status = status;
}

std::string command_transcribe(
                whisper_context * ctx,
                const whisper_full_params & wparams,
                const std::vector<float> & pcmf32,
                float & logprob_min,
                float & logprob_sum,
                int & n_tokens,
                int64_t & t_ms) {
    const auto t_start = std::chrono::high_resolution_clock::now();

    logprob_min = 0.0f;
    logprob_sum = 0.0f;
    n_tokens    = 0;
    t_ms = 0;

    if (whisper_full(ctx, wparams, pcmf32.data(), pcmf32.size()) != 0) {
        return "";
    }

    std::string result;

    const int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) {
        const char * text = whisper_full_get_segment_text(ctx, i);

        result += text;

        const int n = whisper_full_n_tokens(ctx, i);
        for (int j = 0; j < n; ++j) {
            const auto token = whisper_full_get_token_data(ctx, i, j);

            if(token.plog > 0.0f) exit(0); // todo: check for emscripten
            logprob_min = std::min(logprob_min, token.plog);
            logprob_sum += token.plog;
            ++n_tokens;
        }
    }

    const auto t_end = std::chrono::high_resolution_clock::now();
    t_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count();

    return result;
}

void command_get_audio(int ms, int sample_rate, std::vector<float> & audio) {
    const int64_t n_samples = (ms * sample_rate) / 1000;

    int64_t n_take = 0;
    if (n_samples > (int) g_pcmf32.size()) {
        n_take = g_pcmf32.size();
    } else {
        n_take = n_samples;
    }

    audio.resize(n_take);
    std::copy(g_pcmf32.end() - n_take, g_pcmf32.end(), audio.begin());
}

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


class Board {
public:
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
        static_assert(pieceNames.size() == Piece::Taken, "Mismatch between piece names and types");

        enum Colors {
            Black,
            White
        };

        Types type;
        Colors color;
        int pos;
    };


    std::array<Piece, 16> blackPieces = {{
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
    }};

    std::array<Piece, 16> whitePieces = {{
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
    }};

    using BB = std::array<Piece*, 64>;
    BB board = {{
        &whitePieces[ 8], &whitePieces[ 9], &whitePieces[10], &whitePieces[11], &whitePieces[12], &whitePieces[13], &whitePieces[14], &whitePieces[15],
        &whitePieces[ 0], &whitePieces[ 1], &whitePieces[ 2], &whitePieces[ 3], &whitePieces[ 4], &whitePieces[ 5], &whitePieces[ 6], &whitePieces[ 7],
        nullptr,          nullptr,          nullptr,          nullptr,          nullptr,          nullptr,          nullptr,          nullptr,
        nullptr,          nullptr,          nullptr,          nullptr,          nullptr,          nullptr,          nullptr,          nullptr,
        &blackPieces[ 0], &blackPieces[ 1], &blackPieces[ 2], &blackPieces[ 3], &blackPieces[ 4], &blackPieces[ 5], &blackPieces[ 6], &blackPieces[ 7],
        &blackPieces[ 8], &blackPieces[ 9], &blackPieces[10], &blackPieces[11], &blackPieces[12], &blackPieces[13], &blackPieces[14], &blackPieces[15],
    }};

    bool checkNext(const Piece& piece, int pos, bool kingCheck = false) {
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


    int moveCount = 0;


    void addMoves(const std::string& t) {

        std::vector<std::string> moves;
        size_t cur = 0;
        size_t last = 0;
        while (cur != std::string::npos) {
            cur = t.find(',', last);
            moves.push_back(t.substr(last, cur));
            last = cur + 1;
        }

        // fixme: lookup depends on grammar
        int count = moveCount;
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
                m_pendingMoves.push_back({&pieces[pieceIndex], posIndex});
            }
        }
    }

    std::string stringifyMoves() {
        std::string res;
        for (auto& m : m_pendingMoves) {
            res.append(positions[m.first->pos]);
            res.push_back('-');
            res.append(positions[m.second]);
            res.push_back(' ');
        }
        if (!res.empty()) res.pop_back();
        return res;
    }

    void commitMoves() {
        for (auto& m : m_pendingMoves) {
            if (board[m.second]) board[m.second]->type = Piece::Taken;
            board[m.first->pos] = nullptr;
            m.first->pos = m.second;
            board[m.second] = m.first;
        }
        m_pendingMoves.clear();
    }

    std::vector<std::pair<Piece*, int>> m_pendingMoves;
};

Board g_board;

void command_main(size_t index) {
    command_set_status("loading data ...");

    struct whisper_full_params wparams = whisper_full_default_params(whisper_sampling_strategy::WHISPER_SAMPLING_GREEDY);

    wparams.n_threads        = std::min(N_THREAD, (int) std::thread::hardware_concurrency());
    wparams.offset_ms        = 0;
    wparams.translate        = false;
    wparams.no_context       = true;
    wparams.single_segment   = true;
    wparams.print_realtime   = false;
    wparams.print_progress   = false;
    wparams.print_timestamps = true;
    wparams.print_special    = false;

    wparams.max_tokens       = 32;
    // wparams.audio_ctx        = 768; // partial encoder context for better performance

    wparams.temperature     = 0.4f;
    wparams.temperature_inc = 1.0f;
    wparams.greedy.best_of  = 1;

    wparams.beam_search.beam_size = 5;

    wparams.language         = "en";

    printf("command: using %d threads\n", wparams.n_threads);

    bool have_prompt  = false;
    bool ask_prompt   = true;
    bool print_energy = false;

    float logprob_min0 = 0.0f;
    float logprob_min  = 0.0f;

    float logprob_sum0 = 0.0f;
    float logprob_sum  = 0.0f;

    int n_tokens0 = 0;
    int n_tokens  = 0;

    std::vector<float> pcmf32_cur;
    std::vector<float> pcmf32_prompt;

    // todo: grammar to be based on js input
    const std::string k_prompt = "rook to b4, f3,";
    wparams.initial_prompt = "d4 d5 knight to c3, pawn to a1, bishop to b2 king e8,";

    auto grammar_parsed = grammar_parser::parse(
"\n"
"root   ::= init move move? move? \".\"\n"
"prompt ::= init \".\"\n"
"\n"
"# leading space is very important!\n"
"init ::= \" rook to b4, f3\"\n"
"\n"
"move ::= \", \" ((piece | pawn | king) \" \" \"to \"?)? [a-h] [1-8]\n"
"\n"
"piece ::= \"bishop\" | \"rook\" | \"knight\" | \"queen\"\n"
"king  ::= \"king\"\n"
"pawn  ::= \"pawn\"\n"
"\n"
    );
    auto grammar_rules = grammar_parsed.c_rules();

    if (grammar_parsed.rules.empty()) {
        fprintf(stdout, "%s: Failed to parse grammar ...\n", __func__);
    }
    else {
        wparams.grammar_rules   = grammar_rules.data();
        wparams.n_grammar_rules = grammar_rules.size();
        wparams.grammar_penalty = 100.0;
    }

    // whisper context
    auto & ctx = g_contexts[index];

    const int32_t vad_ms     = 2000;
    const int32_t prompt_ms  = 5000;
    const int32_t command_ms = 4000;

    const float vad_thold  = 0.1f;
    const float freq_thold = -1.0f;

    while (g_running) {
        // delay
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        if (ask_prompt) {
            fprintf(stdout, "\n");
            fprintf(stdout, "%s: Say the following phrase: '%s%s%s'\n", __func__, "\033[1m", k_prompt.c_str(), "\033[0m");
            fprintf(stdout, "\n");

            {
                char txt[1024];
                snprintf(txt, sizeof(txt), "Say the following phrase: '%s'", k_prompt.c_str());
                command_set_status(txt);
            }

            ask_prompt = false;
        }

        int64_t t_ms = 0;

        {
            command_get_audio(vad_ms, WHISPER_SAMPLE_RATE, pcmf32_cur);

            if (::vad_simple(pcmf32_cur, WHISPER_SAMPLE_RATE, 1000, vad_thold, freq_thold, print_energy)) {
                fprintf(stdout, "%s: Speech detected! Processing ...\n", __func__);
                command_set_status("Speech detected! Processing ...");

                if (!have_prompt) {
                    command_get_audio(prompt_ms, WHISPER_SAMPLE_RATE, pcmf32_cur);

                    wparams.i_start_rule    = grammar_parsed.symbol_ids.at("prompt");
                    const auto txt = ::trim(::command_transcribe(ctx, wparams, pcmf32_cur, logprob_min, logprob_sum, n_tokens, t_ms));

                    fprintf(stdout, "%s: Heard '%s%s%s', (t = %d ms)\n", __func__, "\033[1m", txt.c_str(), "\033[0m", (int) t_ms);

                    const float sim = similarity(txt, k_prompt);

                    if (txt.length() < 0.8*k_prompt.length() || txt.length() > 1.2*k_prompt.length() || sim < 0.8f) {
                        fprintf(stdout, "%s: WARNING: prompt not recognized, try again\n", __func__);
                        ask_prompt = true;
                    } else {
                        fprintf(stdout, "\n");
                        fprintf(stdout, "%s: The prompt has been recognized!\n", __func__);
                        fprintf(stdout, "%s: Waiting for voice commands ...\n", __func__);
                        fprintf(stdout, "\n");

                        {
                            char txt[1024];
                            snprintf(txt, sizeof(txt), "Success! Waiting for voice commands ...");
                            command_set_status(txt);
                        }

                        // save the audio for the prompt
                        pcmf32_prompt = pcmf32_cur;
                        have_prompt = true;
                    }
                } else {
                    command_get_audio(command_ms, WHISPER_SAMPLE_RATE, pcmf32_cur);

                    // prepend 3 second of silence
                    pcmf32_cur.insert(pcmf32_cur.begin(), 3*WHISPER_SAMPLE_RATE, 0.0f);

                    // prepend the prompt audio
                    pcmf32_cur.insert(pcmf32_cur.begin(), pcmf32_prompt.begin(), pcmf32_prompt.end());

                    wparams.i_start_rule    = grammar_parsed.symbol_ids.at("root");
                    const auto txt = ::trim(::command_transcribe(ctx, wparams, pcmf32_cur, logprob_min, logprob_sum, n_tokens, t_ms));

                    const float p = 100.0f * std::exp(logprob_min);

                    fprintf(stdout, "%s: heard '%s'\n", __func__, txt.c_str());

                    // find the prompt in the text
                    float best_sim = 0.0f;
                    size_t best_len = 0;
                    for (int n = 0.8*k_prompt.size(); n <= 1.2*k_prompt.size(); ++n) {
                        if (n >= int(txt.size())) {
                            break;
                        }

                        const auto prompt = txt.substr(0, n);

                        const float sim = similarity(prompt, k_prompt);

                        //fprintf(stderr, "%s: prompt = '%s', sim = %f\n", __func__, prompt.c_str(), sim);

                        if (sim > best_sim) {
                            best_sim = sim;
                            best_len = n;
                        }
                    }

                    fprintf(stdout, "%s:   DEBUG: txt = '%s', prob = %.2f%%\n", __func__, txt.c_str(), p);
                    std::string command = ::trim(txt.substr(best_len));

                    fprintf(stdout, "%s: Command '%s%s%s', (t = %d ms)\n", __func__, "\033[1m", command.c_str(), "\033[0m", (int) t_ms);
                    fprintf(stdout, "\n");

                    {
                        char txt[1024];
                        snprintf(txt, sizeof(txt), "Command '%s', (t = %d ms)", command.c_str(), (int) t_ms);
                        command_set_status(txt);
                    }
                    {
                        std::lock_guard<std::mutex> lock(g_mutex);
                        if (!command.empty()) {
                            g_board.addMoves(command);
                        }
                        g_transcribed = std::move(command);
                    }
                }

                g_pcmf32.clear();
            }
        }
    }

    if (index < g_contexts.size()) {
        whisper_free(g_contexts[index]);
        g_contexts[index] = nullptr;
    }
}

EMSCRIPTEN_BINDINGS(command) {
    emscripten::function("init", emscripten::optional_override([](const std::string & path_model) {
        for (size_t i = 0; i < g_contexts.size(); ++i) {
            if (g_contexts[i] == nullptr) {
                g_contexts[i] = whisper_init_from_file_with_params(path_model.c_str(), whisper_context_default_params());
                if (g_contexts[i] != nullptr) {
                    g_running = true;
                    if (g_worker.joinable()) {
                        g_worker.join();
                    }
                    g_worker = std::thread([i]() {
                        command_main(i);
                    });

                    return i + 1;
                } else {
                    return (size_t) 0;
                }
            }
        }

        return (size_t) 0;
    }));

    emscripten::function("free", emscripten::optional_override([](size_t index) {
        if (g_running) {
            g_running = false;
        }
    }));

    emscripten::function("set_audio", emscripten::optional_override([](size_t index, const emscripten::val & audio) {
        --index;

        if (index >= g_contexts.size()) {
            return -1;
        }

        if (g_contexts[index] == nullptr) {
            return -2;
        }

        {
            std::lock_guard<std::mutex> lock(g_mutex);
            const int n = audio["length"].as<int>();

            emscripten::val heap = emscripten::val::module_property("HEAPU8");
            emscripten::val memory = heap["buffer"];

            g_pcmf32.resize(n);

            emscripten::val memoryView = audio["constructor"].new_(memory, reinterpret_cast<uintptr_t>(g_pcmf32.data()), n);
            memoryView.call<void>("set", audio);
        }

        return 0;
    }));

    emscripten::function("get_transcribed", emscripten::optional_override([]() {
        std::string transcribed;

        {
            std::lock_guard<std::mutex> lock(g_mutex);
            transcribed = std::move(g_transcribed);
        }

        return transcribed;
    }));


    emscripten::function("get_moves", emscripten::optional_override([]() {
        std::string moves;

        {
            std::lock_guard<std::mutex> lock(g_mutex);
            moves = g_board.stringifyMoves();

            fprintf(stdout, "%s: Moves '%s%s%s'\n", __func__, "\033[1m", moves.c_str(), "\033[0m");
        }

        return moves;
    }));

    emscripten::function("commit_moves", emscripten::optional_override([]() {
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_board.commitMoves();
        }

    }));

    emscripten::function("discard_moves", emscripten::optional_override([]() {
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_board.m_pendingMoves.clear();
        }

    }));

    emscripten::function("get_status", emscripten::optional_override([]() {
        std::string status;

        {
            std::lock_guard<std::mutex> lock(g_mutex);
            status = g_status_forced.empty() ? g_status : g_status_forced;
        }

        return status;
    }));

    emscripten::function("set_status", emscripten::optional_override([](const std::string & status) {
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_status_forced = status;
        }
    }));
}
