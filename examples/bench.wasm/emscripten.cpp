#include "whisper.h"

#include <emscripten.h>
#include <emscripten/bind.h>

#include <cmath>
#include <string>
#include <thread>
#include <vector>

constexpr int N_THREAD = 8;

whisper_context * g_context;
std::vector<struct whisper_state *> g_states(4, nullptr);

std::thread g_worker;

void bench_main(size_t index) {
    const int n_threads = std::min(N_THREAD, (int) std::thread::hardware_concurrency());

    // whisper context
    whisper_state * state = g_states[index];

    fprintf(stderr, "%s: running benchmark with %d threads - please wait...\n", __func__, n_threads);

    if (int ret = whisper_set_mel(state, nullptr, 0, WHISPER_N_MEL)) {
        fprintf(stderr, "error: failed to set mel: %d\n", ret);
        return;
    }

    {
        fprintf(stderr, "\n");
        fprintf(stderr, "system_info: n_threads = %d / %d | %s\n", n_threads, std::thread::hardware_concurrency(), whisper_print_system_info());
    }

    if (int ret = whisper_encode(g_context, state, 0, n_threads) != 0) {
        fprintf(stderr, "error: failed to encode model: %d\n", ret);
        return;
    }

    whisper_print_timings(g_context, state);

    fprintf(stderr, "\n");
    fprintf(stderr, "If you wish, you can submit these results here:\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  https://github.com/ggerganov/whisper.cpp/issues/89\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Please include the following information:\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  - CPU model\n");
    fprintf(stderr, "  - Operating system\n");
    fprintf(stderr, "  - Browser\n");
    fprintf(stderr, "\n");
}

EMSCRIPTEN_BINDINGS(bench) {
    emscripten::function("init", emscripten::optional_override([](const std::string & path_model) {
        if(g_context == nullptr) {
            g_context = whisper_init_from_file(path_model.c_str());
        }

        for (size_t i = 0; i < g_states.size(); ++i) {
            if (g_states[i] == nullptr) {
                g_states[i] = whisper_init_state(g_context);
                if (g_states[i] != nullptr) {
                    if (g_worker.joinable()) {
                        g_worker.join();
                    }
                    g_worker = std::thread([i]() {
                        bench_main(i);
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
        if (index < g_states.size()) {
            whisper_free_state(g_states[index]);
            g_states[index] = nullptr;
        }
        whisper_free(g_context);
        g_context = nullptr;
    }));
}
