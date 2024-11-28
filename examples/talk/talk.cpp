// Talk with AI
//

#include "common-sdl.h"

#include "ggml-backend.h"
#include "ggml-alloc.h"
#include "common.h"
#include "whisper.h"
#include "gpt-2.h"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <regex>
#include <string>
#include <thread>
#include <vector>
#include <regex>

// command-line parameters
struct whisper_params {
    int32_t n_threads  = std::min(4, (int32_t) std::thread::hardware_concurrency());
    int32_t voice_ms   = 10000;
    int32_t capture_id = -1;
    int32_t max_tokens = 32;
    int32_t audio_ctx  = 0;

    float vad_thold    = 0.6f;
    float freq_thold   = 100.0f;

    bool translate     = false;
    bool print_special = false;
    bool print_energy  = false;
    bool no_timestamps = true;
    bool use_gpu       = true;
    bool flash_attn    = false;

    std::string person    = "Santa";
    std::string language  = "en";
    std::string model_wsp = "models/ggml-base.en.bin";
    std::string model_gpt = "models/ggml-gpt-2-117M.bin";
    std::string speak     = "./examples/talk/speak";
    std::string speak_file= "./examples/talk/to_speak.txt";
    std::string fname_out;
};

void whisper_print_usage(int argc, char ** argv, const whisper_params & params);

static bool whisper_params_parse(int argc, char ** argv, whisper_params & params) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            whisper_print_usage(argc, argv, params);
            exit(0);
        }
        else if (arg == "-t"   || arg == "--threads")       { params.n_threads     = std::stoi(argv[++i]); }
        else if (arg == "-vms" || arg == "--voice-ms")      { params.voice_ms      = std::stoi(argv[++i]); }
        else if (arg == "-c"   || arg == "--capture")       { params.capture_id    = std::stoi(argv[++i]); }
        else if (arg == "-mt"  || arg == "--max-tokens")    { params.max_tokens    = std::stoi(argv[++i]); }
        else if (arg == "-ac"  || arg == "--audio-ctx")     { params.audio_ctx     = std::stoi(argv[++i]); }
        else if (arg == "-vth" || arg == "--vad-thold")     { params.vad_thold     = std::stof(argv[++i]); }
        else if (arg == "-fth" || arg == "--freq-thold")    { params.freq_thold    = std::stof(argv[++i]); }
        else if (arg == "-tr"  || arg == "--translate")     { params.translate     = true; }
        else if (arg == "-ps"  || arg == "--print-special") { params.print_special = true; }
        else if (arg == "-pe"  || arg == "--print-energy")  { params.print_energy  = true; }
        else if (arg == "-ng"  || arg == "--no-gpu")        { params.use_gpu       = false; }
        else if (arg == "-fa"  || arg == "--flash-attn")    { params.flash_attn    = true; }
        else if (arg == "-p"   || arg == "--person")        { params.person        = argv[++i]; }
        else if (arg == "-l"   || arg == "--language")      { params.language      = argv[++i]; }
        else if (arg == "-mw"  || arg == "--model-whisper") { params.model_wsp     = argv[++i]; }
        else if (arg == "-mg"  || arg == "--model-gpt")     { params.model_gpt     = argv[++i]; }
        else if (arg == "-s"   || arg == "--speak")         { params.speak         = argv[++i]; }
        else if (arg == "-sf"  || arg == "--speak_file")    { params.speak_file    = argv[++i]; }
        else if (arg == "-f"   || arg == "--file")          { params.fname_out     = argv[++i]; }
        else {
            fprintf(stderr, "error: unknown argument: %s\n", arg.c_str());
            whisper_print_usage(argc, argv, params);
            exit(0);
        }
    }

    return true;
}

void whisper_print_usage(int /*argc*/, char ** argv, const whisper_params & params) {
    fprintf(stderr, "\n");
    fprintf(stderr, "usage: %s [options]\n", argv[0]);
    fprintf(stderr, "\n");
    fprintf(stderr, "options:\n");
    fprintf(stderr, "  -h,       --help          [default] show this help message and exit\n");
    fprintf(stderr, "  -t N,     --threads N     [%-7d] number of threads to use during computation\n", params.n_threads);
    fprintf(stderr, "  -vms N,   --voice-ms N    [%-7d] voice duration in milliseconds\n",              params.voice_ms);
    fprintf(stderr, "  -c ID,    --capture ID    [%-7d] capture device ID\n",                           params.capture_id);
    fprintf(stderr, "  -mt N,    --max-tokens N  [%-7d] maximum number of tokens per audio chunk\n",    params.max_tokens);
    fprintf(stderr, "  -ac N,    --audio-ctx N   [%-7d] audio context size (0 - all)\n",                params.audio_ctx);
    fprintf(stderr, "  -vth N,   --vad-thold N   [%-7.2f] voice activity detection threshold\n",        params.vad_thold);
    fprintf(stderr, "  -fth N,   --freq-thold N  [%-7.2f] high-pass frequency cutoff\n",                params.freq_thold);
    fprintf(stderr, "  -tr,      --translate     [%-7s] translate from source language to english\n",   params.translate ? "true" : "false");
    fprintf(stderr, "  -ps,      --print-special [%-7s] print special tokens\n",                        params.print_special ? "true" : "false");
    fprintf(stderr, "  -pe,      --print-energy  [%-7s] print sound energy (for debugging)\n",          params.print_energy ? "true" : "false");
    fprintf(stderr, "  -ng,      --no-gpu        [%-7s] disable GPU\n",                                 params.use_gpu ? "false" : "true");
    fprintf(stderr, "  -fa,      --flash-attn    [%-7s] flash attention\n",                             params.flash_attn ? "true" : "false");
    fprintf(stderr, "  -p NAME,  --person NAME   [%-7s] person name (for prompt selection)\n",          params.person.c_str());
    fprintf(stderr, "  -l LANG,  --language LANG [%-7s] spoken language\n",                             params.language.c_str());
    fprintf(stderr, "  -mw FILE, --model-whisper [%-7s] whisper model file\n",                          params.model_wsp.c_str());
    fprintf(stderr, "  -mg FILE, --model-gpt     [%-7s] gpt model file\n",                              params.model_gpt.c_str());
    fprintf(stderr, "  -s FILE,  --speak TEXT    [%-7s] command for TTS\n",                             params.speak.c_str());
    fprintf(stderr, "  -sf FILE, --speak_file    [%-7s] file to pass to TTS\n",                         params.speak_file.c_str());
    fprintf(stderr, "  -f FNAME, --file FNAME    [%-7s] text output file name\n",                       params.fname_out.c_str());
    fprintf(stderr, "\n");
}

static std::string transcribe(whisper_context * ctx, const whisper_params & params, const std::vector<float> & pcmf32, float & prob, int64_t & t_ms) {
    const auto t_start = std::chrono::high_resolution_clock::now();

    prob = 0.0f;
    t_ms = 0;

    whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);

    wparams.print_progress   = false;
    wparams.print_special    = params.print_special;
    wparams.print_realtime   = false;
    wparams.print_timestamps = !params.no_timestamps;
    wparams.translate        = params.translate;
    wparams.no_context       = true;
    wparams.single_segment   = true;
    wparams.max_tokens       = params.max_tokens;
    wparams.language         = params.language.c_str();
    wparams.n_threads        = params.n_threads;

    wparams.audio_ctx        = params.audio_ctx;

    if (whisper_full(ctx, wparams, pcmf32.data(), pcmf32.size()) != 0) {
        return "";
    }

    int prob_n = 0;
    std::string result;

    const int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) {
        const char * text = whisper_full_get_segment_text(ctx, i);

        result += text;

        const int n_tokens = whisper_full_n_tokens(ctx, i);
        for (int j = 0; j < n_tokens; ++j) {
            const auto token = whisper_full_get_token_data(ctx, i, j);

            prob += token.p;
            ++prob_n;
        }
    }

    if (prob_n > 0) {
        prob /= prob_n;
    }

    const auto t_end = std::chrono::high_resolution_clock::now();
    t_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count();

    return result;
}

const std::string k_prompt =
R"(This is a dialogue between {0} (A) and a person (B). The dialogue so far is:

B: Hello {0}, how are you?
A: I'm fine, thank you.
{1}
Here is how {0} (A) continues the dialogue:

A:)";

int main(int argc, char ** argv) {
    whisper_params params;

    if (whisper_params_parse(argc, argv, params) == false) {
        return 1;
    }

    if (whisper_lang_id(params.language.c_str()) == -1) {
        fprintf(stderr, "error: unknown language '%s'\n", params.language.c_str());
        whisper_print_usage(argc, argv, params);
        exit(0);
    }

    // whisper init
    struct whisper_context_params cparams = whisper_context_default_params();

    cparams.use_gpu    = params.use_gpu;
    cparams.flash_attn = params.flash_attn;

    struct whisper_context * ctx_wsp = whisper_init_from_file_with_params(params.model_wsp.c_str(), cparams);

    // gpt init

    struct gpt2_context * ctx_gpt = gpt2_init(params.model_gpt.c_str());

    // print some info about the processing
    {
        fprintf(stderr, "\n");
        if (!whisper_is_multilingual(ctx_wsp)) {
            if (params.language != "en" || params.translate) {
                params.language = "en";
                params.translate = false;
                fprintf(stderr, "%s: WARNING: model is not multilingual, ignoring language and translation options\n", __func__);
            }
        }
        fprintf(stderr, "%s: processing, %d threads, lang = %s, task = %s, timestamps = %d ...\n",
                __func__,
                params.n_threads,
                params.language.c_str(),
                params.translate ? "translate" : "transcribe",
                params.no_timestamps ? 0 : 1);

        fprintf(stderr, "\n");
    }


    // init audio

    audio_async audio(30*1000);
    if (!audio.init(params.capture_id, WHISPER_SAMPLE_RATE))
    {
        fprintf(stderr, "%s: audio.init() failed!\n", __func__);
        return 1;
    }
    printf("audio init successful....\n");

    audio.resume();

    int n_iter = 0;

    bool is_running  = true;
    bool force_speak = false;

    float prob0 = 0.0f;

    std::vector<float> pcmf32_cur;
    std::vector<float> pcmf32_prompt;

    gpt2_set_prompt(ctx_gpt, "");
    ggml_gallocr_t allocr = NULL;
    // allocate the compute buffer
    {
        // create a graph allocator with the backend's default buffer type
        allocr = ggml_gallocr_new(ggml_backend_get_default_buffer_type(ctx_gpt->model.backend));
    }

    const int voice_id = rand()%6;

    fprintf(stderr, "gpt-2: prompt:\n");
    fprintf(stderr, "========================\n\n");
    fprintf(stderr, "%s\n", ::replace(k_prompt, "{0}", params.person).c_str());
    fprintf(stderr, "========================\n\n");

    // main loop
    while (is_running) {
        // handle Ctrl + C
        is_running = sdl_poll_events();

        if (!is_running) {
            break;
        }

        // delay
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        int64_t t_ms = 0;

        {
            audio.get(2000, pcmf32_cur);

            if (::vad_simple(pcmf32_cur, WHISPER_SAMPLE_RATE, 1250, params.vad_thold, params.freq_thold, params.print_energy) || force_speak) {
                fprintf(stdout, "%s: Speech detected! Processing ...\n", __func__);

                audio.get(params.voice_ms, pcmf32_cur);

                std::string text_heard;

                if (!force_speak) {
                    text_heard = ::trim(::transcribe(ctx_wsp, params, pcmf32_cur, prob0, t_ms));
                }

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
                text_heard = text_heard.substr(0, text_heard.find_first_of('\n'));

                // remove leading and trailing whitespace
                text_heard = std::regex_replace(text_heard, std::regex("^\\s+"), "");
                text_heard = std::regex_replace(text_heard, std::regex("\\s+$"), "");

                const std::vector<gpt_vocab::id> tokens = gpt2_tokenize(ctx_gpt, text_heard.c_str());

                if (text_heard.empty() || tokens.empty() || force_speak) {
                    fprintf(stdout, "%s: Heard nothing, skipping ...\n", __func__);
                    audio.clear();

                    continue;
                }

                force_speak = false;

                fprintf(stdout, "%s: Heard '%s%s%s', (t = %d ms)\n", __func__, "\033[1m", text_heard.c_str(), "\033[0m", (int) t_ms);

                std::string prompt_base = gpt2_get_prompt(ctx_gpt);

                std::string text_to_speak;

                {
                    prompt_base += "B: " + text_heard + "\n";

                    std::string prompt = ::replace(::replace(k_prompt, "{0}", params.person), "{1}", prompt_base);

                    text_to_speak = gpt2_gen_text(ctx_gpt, prompt.c_str(), params.max_tokens, allocr);
                    //text_to_speak = std::regex_replace(text_to_speak, std::regex("[^a-zA-Z0-9\\.,\\?!\\s\\:\\'\\-]"), "");
                    text_to_speak = text_to_speak.substr(0, text_to_speak.find_first_of('\n'));

                    // remove first 2 lines of base prompt
                    if (n_iter > 4) {
                        {
                            const size_t pos = prompt_base.find_first_of('\n');
                            if (pos != std::string::npos) {
                                prompt_base = prompt_base.substr(pos + 1);
                            }
                        }
                        {
                            const size_t pos = prompt_base.find_first_of('\n');
                            if (pos != std::string::npos) {
                                prompt_base = prompt_base.substr(pos + 1);
                            }
                        }
                    }

                    prompt_base += "A:" + text_to_speak + "\n";

                    {
                        prompt = ::replace(::replace(k_prompt, "{0}", params.person), "{1}", prompt_base);

                        printf("===============\n");
                        printf("prompt:\n");
                        printf("%s\n", prompt.c_str());
                        printf("===============\n");
                    }
                }

                //printf("========================\n");
                //printf("gpt-2: prompt_base:\n%s\n", prompt_base.c_str());
                //printf("========================\n");

                gpt2_set_prompt(ctx_gpt, prompt_base.c_str());

                text_to_speak = ::replace(text_to_speak, params.person + ": ", "");
                speak_with_file(params.speak, text_to_speak, params.speak_file, voice_id);

                audio.clear();

                ++n_iter;
            }
        }
    }

    audio.pause();

    whisper_print_timings(ctx_wsp);
    whisper_free(ctx_wsp);

    return 0;
}
