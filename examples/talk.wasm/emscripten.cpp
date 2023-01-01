#include "ggml.h"
#include "gpt-2.h"
#include "whisper.h"

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

struct gpt2_context * g_gpt2;
std::vector<struct whisper_context *> g_contexts(4, nullptr);

std::mutex g_mutex;
std::thread g_worker;
std::atomic<bool> g_running(false);

bool g_force_speak = false;
std::string g_text_to_speak = "";
std::string g_status = "";
std::string g_status_forced = "";

std::vector<float> g_pcmf32;

std::string to_timestamp(int64_t t) {
    int64_t sec = t/100;
    int64_t msec = t - sec*100;
    int64_t min = sec/60;
    sec = sec - min*60;

    char buf[32];
    snprintf(buf, sizeof(buf), "%02d:%02d.%03d", (int) min, (int) sec, (int) msec);

    return std::string(buf);
}

void talk_set_status(const std::string & status) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_status = status;
}

void talk_main(size_t index) {
    talk_set_status("loading data ...");

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
    wparams.audio_ctx        = 768; // partial encoder context for better performance

    wparams.language         = "en";

    g_gpt2 = gpt2_init("gpt-2.bin");

    printf("talk: using %d threads\n", wparams.n_threads);

    std::vector<float> pcmf32;

    // whisper context
    auto & ctx = g_contexts[index];

    const int64_t step_samples   = 2*WHISPER_SAMPLE_RATE;
    const int64_t window_samples = 9*WHISPER_SAMPLE_RATE;
    const int64_t step_ms        = (step_samples*1000)/WHISPER_SAMPLE_RATE;

    auto t_last = std::chrono::high_resolution_clock::now();

    talk_set_status("listening ...");

    while (g_running) {

        const auto t_now = std::chrono::high_resolution_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(t_now - t_last).count() < step_ms) {
            {
                std::lock_guard<std::mutex> lock(g_mutex);
                g_pcmf32.clear();
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        talk_set_status("listening ...");

        {
            std::unique_lock<std::mutex> lock(g_mutex);

            if (g_pcmf32.size() < step_samples) {
                lock.unlock();

                std::this_thread::sleep_for(std::chrono::milliseconds(10));

                continue;
            }

            pcmf32 = std::vector<float>(g_pcmf32.end() - std::min((int64_t) g_pcmf32.size(), window_samples), g_pcmf32.end());
        }

        // VAD: if energy in during last second is above threshold, then skip
        {
            float energy_all = 0.0f;
            float energy_1s  = 0.0f;

            for (size_t i = 0; i < pcmf32.size(); i++) {
                energy_all += fabsf(pcmf32[i]);

                if (i >= pcmf32.size() - WHISPER_SAMPLE_RATE) {
                    energy_1s += fabsf(pcmf32[i]);
                }
            }

            energy_all /= pcmf32.size();
            energy_1s  /= WHISPER_SAMPLE_RATE;

            if (energy_1s > 0.1f*energy_all && !g_force_speak) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }
        }

        talk_set_status("processing audio (whisper)...");

        t_last = t_now;

        if (!g_force_speak) {
            const auto t_start = std::chrono::high_resolution_clock::now();

            int ret = whisper_full(ctx, wparams, pcmf32.data(), pcmf32.size());
            if (ret != 0) {
                printf("whisper_full() failed: %d\n", ret);
                break;
            }

            const auto t_end = std::chrono::high_resolution_clock::now();

            printf("whisper_full() returned %d in %f seconds\n", ret, std::chrono::duration<double>(t_end - t_start).count());
        }

        {
            std::string text_heard;

            if (!g_force_speak) {
                const int n_segments = whisper_full_n_segments(ctx);
                for (int i = n_segments - 1; i < n_segments; ++i) {
                    const char * text = whisper_full_get_segment_text(ctx, i);

                    const int64_t t0 = whisper_full_get_segment_t0(ctx, i);
                    const int64_t t1 = whisper_full_get_segment_t1(ctx, i);

                    printf ("[%s --> %s]  %s\n", to_timestamp(t0).c_str(), to_timestamp(t1).c_str(), text);

                    text_heard += text;
                }
            }

            g_force_speak = false;

            // remove text between brackets using regex
            {
                std::regex re("\\[.*?\\]");
                text_heard = std::regex_replace(text_heard, re, "");
            }

            // remove text between brackets using regex
            {
                std::regex re("\\(.*?\\)");
                text_heard = std::regex_replace(text_heard, re, "");
            }

            // remove all characters, except for letters, numbers, punctuation and ':', '\'', '-', ' '
            text_heard = std::regex_replace(text_heard, std::regex("[^a-zA-Z0-9\\.,\\?!\\s\\:\\'\\-]"), "");

            // take first line
            text_heard = text_heard.substr(0, text_heard.find_first_of("\n"));

            // remove leading and trailing whitespace
            text_heard = std::regex_replace(text_heard, std::regex("^\\s+"), "");
            text_heard = std::regex_replace(text_heard, std::regex("\\s+$"), "");

            talk_set_status("'" + text_heard + "' - thinking how to respond (gpt-2) ...");

            const std::vector<gpt_vocab::id> tokens = gpt2_tokenize(g_gpt2, text_heard.c_str());

            printf("whisper: number of tokens: %d, '%s'\n", (int) tokens.size(), text_heard.c_str());

            std::string text_to_speak;
            std::string prompt_base;

            {
                std::lock_guard<std::mutex> lock(g_mutex);
                prompt_base = gpt2_get_prompt(g_gpt2);
            }

            if (tokens.size() > 0) {
                text_to_speak = gpt2_gen_text(g_gpt2, (prompt_base + text_heard + "\n").c_str(), 32);
                text_to_speak = std::regex_replace(text_to_speak, std::regex("[^a-zA-Z0-9\\.,\\?!\\s\\:\\'\\-]"), "");
                text_to_speak = text_to_speak.substr(0, text_to_speak.find_first_of("\n"));

                std::lock_guard<std::mutex> lock(g_mutex);

                // remove first 2 lines of base prompt
                {
                    const size_t pos = prompt_base.find_first_of("\n");
                    if (pos != std::string::npos) {
                        prompt_base = prompt_base.substr(pos + 1);
                    }
                }
                {
                    const size_t pos = prompt_base.find_first_of("\n");
                    if (pos != std::string::npos) {
                        prompt_base = prompt_base.substr(pos + 1);
                    }
                }
                prompt_base += text_heard + "\n" + text_to_speak + "\n";
            } else {
                text_to_speak = gpt2_gen_text(g_gpt2, prompt_base.c_str(), 32);
                text_to_speak = std::regex_replace(text_to_speak, std::regex("[^a-zA-Z0-9\\.,\\?!\\s\\:\\'\\-]"), "");
                text_to_speak = text_to_speak.substr(0, text_to_speak.find_first_of("\n"));

                std::lock_guard<std::mutex> lock(g_mutex);

                const size_t pos = prompt_base.find_first_of("\n");
                if (pos != std::string::npos) {
                    prompt_base = prompt_base.substr(pos + 1);
                }
                prompt_base += text_to_speak + "\n";
            }

            printf("gpt-2: %s\n", text_to_speak.c_str());

            //printf("========================\n");
            //printf("gpt-2: prompt_base:\n'%s'\n", prompt_base.c_str());
            //printf("========================\n");

            {
                std::lock_guard<std::mutex> lock(g_mutex);
                t_last = std::chrono::high_resolution_clock::now();
                g_text_to_speak = text_to_speak;
                g_pcmf32.clear();
                gpt2_set_prompt(g_gpt2, prompt_base.c_str());
            }

            talk_set_status("speaking ...");
        }
    }

    gpt2_free(g_gpt2);

    if (index < g_contexts.size()) {
        whisper_free(g_contexts[index]);
        g_contexts[index] = nullptr;
    }
}

EMSCRIPTEN_BINDINGS(talk) {
    emscripten::function("init", emscripten::optional_override([](const std::string & path_model) {
        for (size_t i = 0; i < g_contexts.size(); ++i) {
            if (g_contexts[i] == nullptr) {
                g_contexts[i] = whisper_init_from_file(path_model.c_str());
                if (g_contexts[i] != nullptr) {
                    g_running = true;
                    if (g_worker.joinable()) {
                        g_worker.join();
                    }
                    g_worker = std::thread([i]() {
                        talk_main(i);
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

    emscripten::function("force_speak", emscripten::optional_override([](size_t index) {
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_force_speak = true;
        }
    }));

    emscripten::function("get_text_context", emscripten::optional_override([]() {
        std::string text_context;

        {
            std::lock_guard<std::mutex> lock(g_mutex);
            text_context = gpt2_get_prompt(g_gpt2);
        }

        return text_context;
    }));

    emscripten::function("get_text_to_speak", emscripten::optional_override([]() {
        std::string text_to_speak;

        {
            std::lock_guard<std::mutex> lock(g_mutex);
            text_to_speak = std::move(g_text_to_speak);
        }

        return text_to_speak;
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

    emscripten::function("set_prompt", emscripten::optional_override([](const std::string & prompt) {
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            gpt2_set_prompt(g_gpt2, prompt.c_str());
        }
    }));
}
