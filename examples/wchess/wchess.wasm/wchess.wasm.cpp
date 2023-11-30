#include <WChess.h>
#include <emscripten/bind.h>

#include <atomic>
#include <thread>

constexpr int N_THREAD = 8;

std::vector<struct whisper_context *> g_contexts(4, nullptr);

std::mutex  g_mutex;
std::thread g_worker;

std::atomic<bool> g_running(false);

std::string g_status        = "";
std::string g_status_forced = "";
std::string g_moves         = "";

std::vector<float> g_pcmf32;

void set_status(const std::string & status) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_status = status;
}

void set_moves(const std::string & moves) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_moves = moves;
}

void get_audio(int /* ms */, std::vector<float> & audio) {
    std::lock_guard<std::mutex> lock(g_mutex);
    audio = g_pcmf32;
}

bool check_running() {
    return g_running;
}

void clear_audio() {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_pcmf32.clear();
}

void wchess_main(size_t i) {
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
    wparams.no_timestamps    = true;

    wparams.max_tokens       = 32;
    wparams.audio_ctx        = 768; // partial encoder context for better performance

    wparams.temperature     = 0.0f;
    wparams.temperature_inc = 2.0f;
    wparams.greedy.best_of  = 1;

    wparams.beam_search.beam_size = 1;

    wparams.language         = "en";

    wparams.grammar_penalty = 100.0;
    wparams.initial_prompt = "bishop to c3, rook to d4, knight to e5, d4 d5, knight to c3, c3, queen to d4, king b1, pawn to a1, bishop to b2, knight to c3,";

    printf("command: using %d threads\n", wparams.n_threads);

    WChess::callbacks cb;
    cb.set_status = set_status;
    cb.check_running = check_running;
    cb.get_audio = get_audio;
    cb.set_moves = set_moves;
    cb.clear_audio = clear_audio;

    WChess(g_contexts[i], wparams, cb, {}).run();
    if (i < g_contexts.size()) {
        whisper_free(g_contexts[i]);
        g_contexts[i] = nullptr;
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
                        wchess_main(i);
                    });

                    return i + 1;
                } else {
                    return (size_t) 0;
                }
            }
        }

        return (size_t) 0;
    }));

    emscripten::function("free", emscripten::optional_override([](size_t /* index */) {
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

    emscripten::function("get_moves", emscripten::optional_override([]() {
        std::string moves;

        {
            std::lock_guard<std::mutex> lock(g_mutex);
            moves = std::move(g_moves);
        }

        return moves;
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
        std::lock_guard<std::mutex> lock(g_mutex);
        g_status_forced = status;
    }));
}
