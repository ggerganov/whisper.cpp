// Talk with AI
//

#include "common.h"
#include "common-sdl.h"
#include "whisper.h"
#include "llama.h"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <regex>
#include <string>
#include <thread>
#include <vector>
#include <regex>

std::vector<llama_token> llama_tokenize(struct llama_context * ctx, const std::string & text, bool add_bos) {
    // initialize to prompt numer of chars, since n_tokens <= n_prompt_chars
    std::vector<llama_token> res(text.size() + (int)add_bos);
    int n = llama_tokenize(ctx, text.c_str(), res.data(), res.size(), add_bos);
    assert(n >= 0);
    res.resize(n);

    return res;
}

// command-line parameters
struct whisper_params {
    int32_t n_threads  = std::min(4, (int32_t) std::thread::hardware_concurrency());
    int32_t voice_ms   = 10000;
    int32_t capture_id = -1;
    int32_t max_tokens = 32;
    int32_t audio_ctx  = 0;

    int32_t n_parts_llama = -1;

    float vad_thold    = 0.6f;
    float freq_thold   = 100.0f;

    bool speed_up      = false;
    bool translate     = false;
    bool print_special = false;
    bool print_energy  = false;
    bool no_timestamps = true;
    bool verbose_prompt = false;

    std::string person      = "Georgi";
    std::string language    = "en";
    std::string model_wsp   = "models/ggml-base.en.bin";
    std::string model_llama = "models/ggml-llama-7B.bin";
    std::string speak       = "./examples/talk-llama/speak.sh";
    std::string prompt      = "";
    std::string fname_out;
    std::string path_session = "";       // path to file for saving/loading model eval state
};

void whisper_print_usage(int argc, char ** argv, const whisper_params & params);

bool whisper_params_parse(int argc, char ** argv, whisper_params & params) {
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
        else if (arg == "--n-parts-llama")                  { params.n_parts_llama = std::stoi(argv[++i]); }
        else if (arg == "-su"  || arg == "--speed-up")      { params.speed_up      = true; }
        else if (arg == "-tr"  || arg == "--translate")     { params.translate     = true; }
        else if (arg == "-ps"  || arg == "--print-special") { params.print_special = true; }
        else if (arg == "-pe"  || arg == "--print-energy")  { params.print_energy  = true; }
        else if (arg == "--verbose-prompt")                 { params.verbose_prompt = true; }
        else if (arg == "-p"   || arg == "--person")        { params.person        = argv[++i]; }
        else if (arg == "--session")                        { params.path_session  = argv[++i];}
        else if (arg == "-l"   || arg == "--language")      { params.language      = argv[++i]; }
        else if (arg == "-mw"  || arg == "--model-whisper") { params.model_wsp     = argv[++i]; }
        else if (arg == "-ml"  || arg == "--model-llama")   { params.model_llama   = argv[++i]; }
        else if (arg == "-s"   || arg == "--speak")         { params.speak         = argv[++i]; }
        else if (arg == "--prompt-file")                    {
            std::ifstream file(argv[++i]);
            std::copy(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(), back_inserter(params.prompt));
            if (params.prompt.back() == '\n') {
                params.prompt.pop_back();
            }
        }
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
    fprintf(stderr, "  -su,      --speed-up      [%-7s] speed up audio by x2 (reduced accuracy)\n",     params.speed_up ? "true" : "false");
    fprintf(stderr, "  -tr,      --translate     [%-7s] translate from source language to english\n",   params.translate ? "true" : "false");
    fprintf(stderr, "  -ps,      --print-special [%-7s] print special tokens\n",                        params.print_special ? "true" : "false");
    fprintf(stderr, "  -pe,      --print-energy  [%-7s] print sound energy (for debugging)\n",          params.print_energy ? "true" : "false");
    fprintf(stderr, "  -p NAME,  --person NAME   [%-7s] person name (for prompt selection)\n",          params.person.c_str());
    fprintf(stderr, "  -l LANG,  --language LANG [%-7s] spoken language\n",                             params.language.c_str());
    fprintf(stderr, "  -mw FILE, --model-whisper [%-7s] whisper model file\n",                          params.model_wsp.c_str());
    fprintf(stderr, "  -ml FILE, --model-llama   [%-7s] llama model file\n",                            params.model_llama.c_str());
    fprintf(stderr, "  --n-parts-llama N         [%-7d] num parts in llama model file\n",               params.n_parts_llama);
    fprintf(stderr, "  -s FILE,  --speak TEXT    [%-7s] command for TTS\n",                             params.speak.c_str());
    fprintf(stderr, "  --prompt-file FNAME       [%-7s] file with custom prompt to start dialog\n",     "");
    fprintf(stderr, "  --session FNAME       file to cache model state in (may be large!) (default: none)\n");
    fprintf(stderr, "  --verbose-prompt          [%-7s] print prompt at start\n",                       params.verbose_prompt ? "true" : "false");
    fprintf(stderr, "  -f FNAME, --file FNAME    [%-7s] text output file name\n",                       params.fname_out.c_str());
    fprintf(stderr, "\n");
}

std::string transcribe(
        whisper_context * ctx,
        const whisper_params & params,
        const std::vector<float> & pcmf32,
        const std::string prompt_text,
        float & prob,
        int64_t & t_ms) {
    const auto t_start = std::chrono::high_resolution_clock::now();

    prob = 0.0f;
    t_ms = 0;

    std::vector<whisper_token> prompt_tokens;

    whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);

    prompt_tokens.resize(1024);
    prompt_tokens.resize(whisper_tokenize(ctx, prompt_text.c_str(), prompt_tokens.data(), prompt_tokens.size()));

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

    wparams.prompt_tokens    = prompt_tokens.empty() ? nullptr : prompt_tokens.data();
    wparams.prompt_n_tokens  = prompt_tokens.empty() ? 0       : prompt_tokens.size();

    wparams.audio_ctx        = params.audio_ctx;
    wparams.speed_up         = params.speed_up;

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

const std::string k_prompt_whisper = R"(A conversation with a person called {1}.)";

const std::string k_prompt_llama = R"(Text transcript of a never ending dialog, where {0} interacts with an AI assistant named {1}.
{1} is helpful, kind, honest, friendly, good at writing and never fails to answer {0}’s requests immediately and with details and precision.
There are no annotations like (30 seconds passed...) or (to himself), just what {0} and {1} say aloud to each other.
The transcript only includes text, it does not include markup like HTML and Markdown.
{1} responds with short and concise answers.

{0}{4} Hello, {1}!
{1}{4} Hello {0}! How may I help you today?
{0}{4} What time is it?
{1}{4} It is {2} o'clock.
{0}{4} What year is it?
{1}{4} We are in {3}.
{0}{4} What is a cat?
{1}{4} A cat is a domestic species of small carnivorous mammal. It is the only domesticated species in the family Felidae.
{0}{4} Name a color.
{1}{4} Blue
{0}{4})";

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

    struct whisper_context * ctx_wsp = whisper_init_from_file(params.model_wsp.c_str());

    // llama init

    auto lparams = llama_context_default_params();

    // tune these to your liking
    lparams.n_ctx      = 2048;
    lparams.seed       = 1;
    lparams.f16_kv     = true;
    lparams.n_parts    = params.n_parts_llama;

    struct llama_context * ctx_llama = llama_init_from_file(params.model_llama.c_str(), lparams);

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
    if (!audio.init(params.capture_id, WHISPER_SAMPLE_RATE)) {
        fprintf(stderr, "%s: audio.init() failed!\n", __func__);
        return 1;
    }

    audio.resume();

    int n_iter = 0;

    bool is_running  = true;
    bool force_speak = false;

    float prob0 = 0.0f;

    const std::string chat_symb = ":";
    const std::string bot_name  = "LLaMA";

    std::vector<float> pcmf32_cur;
    std::vector<float> pcmf32_prompt;

    const std::string prompt_whisper = ::replace(k_prompt_whisper, "{1}", bot_name);

    // construct the initial prompt for LLaMA inference
    std::string prompt_llama = params.prompt.empty() ? k_prompt_llama : params.prompt;

    // need to have leading ' '
    prompt_llama.insert(0, 1, ' ');

    prompt_llama = ::replace(prompt_llama, "{0}", params.person);
    prompt_llama = ::replace(prompt_llama, "{1}", bot_name);

    {
        // get time string
        std::string time_str;
        {
            time_t t = time(0);
            struct tm * now = localtime(&t);
            char buf[128];
            strftime(buf, sizeof(buf), "%H:%M", now);
            time_str = buf;
        }
        prompt_llama = ::replace(prompt_llama, "{2}", time_str);
    }

    {
        // get year string
        std::string year_str;
        {
            time_t t = time(0);
            struct tm * now = localtime(&t);
            char buf[128];
            strftime(buf, sizeof(buf), "%Y", now);
            year_str = buf;
        }
        prompt_llama = ::replace(prompt_llama, "{3}", year_str);
    }

    prompt_llama = ::replace(prompt_llama, "{4}", chat_symb);

    // init session
    std::string path_session = params.path_session;
    std::vector<llama_token> session_tokens;
    auto embd_inp = ::llama_tokenize(ctx_llama, prompt_llama, true);

    if (!path_session.empty()) {
        fprintf(stderr, "%s: attempting to load saved session from %s\n", __func__, path_session.c_str());

        // fopen to check for existing session
        FILE * fp = std::fopen(path_session.c_str(), "rb");
        if (fp != NULL) {
            std::fclose(fp);

            session_tokens.resize(lparams.n_ctx);
            size_t n_token_count_out = 0;
            if (!llama_load_session_file(ctx_llama, path_session.c_str(), session_tokens.data(), session_tokens.capacity(), &n_token_count_out)) {
                fprintf(stderr, "%s: error: failed to load session file '%s'\n", __func__, path_session.c_str());
                return 1;
            }
            session_tokens.resize(n_token_count_out);
            for (size_t i = 0; i < session_tokens.size(); i++) {
                embd_inp[i] = session_tokens[i];
            }

            fprintf(stderr, "%s: loaded a session with prompt size of %d tokens\n", __func__, (int) session_tokens.size());
        } else {
            fprintf(stderr, "%s: session file does not exist, will create\n", __func__);
        }
    }

    // evaluate the initial prompt

    printf("\n");
    printf("%s : initializing - please wait ...\n", __func__);

    if (llama_eval(ctx_llama, embd_inp.data(), embd_inp.size(), 0, params.n_threads)) {
        fprintf(stderr, "%s : failed to eval\n", __func__);
        return 1;
    }

    if (params.verbose_prompt) {
        fprintf(stdout, "\n");
        fprintf(stdout, "%s", prompt_llama.c_str());
        fflush(stdout);
    }

     // debug message about similarity of saved session, if applicable
    size_t n_matching_session_tokens = 0;
    if (session_tokens.size()) {
        for (llama_token id : session_tokens) {
            if (n_matching_session_tokens >= embd_inp.size() || id != embd_inp[n_matching_session_tokens]) {
                break;
            }
            n_matching_session_tokens++;
        }
        if (n_matching_session_tokens >= embd_inp.size()) {
            fprintf(stderr, "%s: session file has exact match for prompt!\n", __func__);
        } else if (n_matching_session_tokens < (embd_inp.size() / 2)) {
            fprintf(stderr, "%s: warning: session file has low similarity to prompt (%zu / %zu tokens); will mostly be reevaluated\n",
                __func__, n_matching_session_tokens, embd_inp.size());
        } else {
            fprintf(stderr, "%s: session file matches %zu / %zu tokens of prompt\n",
                __func__, n_matching_session_tokens, embd_inp.size());
        }
    }

    // HACK - because session saving incurs a non-negligible delay, for now skip re-saving session
    // if we loaded a session with at least 75% similarity. It's currently just used to speed up the
    // initial prompt so it doesn't need to be an exact match.
    bool need_to_save_session = !path_session.empty() && n_matching_session_tokens < (embd_inp.size() * 3 / 4);

    printf("%s : done! start speaking in the microphone\n", __func__);
    printf("\n");
    printf("%s%s", params.person.c_str(), chat_symb.c_str());
    fflush(stdout);

    // clear audio buffer
    audio.clear();

    // text inference variables
    const int voice_id = 2;
    const int n_keep   = embd_inp.size();
    const int n_ctx    = llama_n_ctx(ctx_llama);

    int n_past = n_keep;
    int n_prev = 64; // TODO arg
    int n_session_consumed = !path_session.empty() && session_tokens.size() > 0 ? session_tokens.size() : 0;

    std::vector<llama_token> embd;

    // reverse prompts for detecting when it's time to stop speaking
    std::vector<std::string> antiprompts = {
        params.person + chat_symb,
    };

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
                //fprintf(stdout, "%s: Speech detected! Processing ...\n", __func__);

                audio.get(params.voice_ms, pcmf32_cur);

                std::string text_heard;

                if (!force_speak) {
                    text_heard = ::trim(::transcribe(ctx_wsp, params, pcmf32_cur, prompt_whisper, prob0, t_ms));
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

                const std::vector<llama_token> tokens = llama_tokenize(ctx_llama, text_heard.c_str(), false);

                if (text_heard.empty() || tokens.empty() || force_speak) {
                    //fprintf(stdout, "%s: Heard nothing, skipping ...\n", __func__);
                    audio.clear();

                    continue;
                }

                force_speak = false;

                text_heard.insert(0, 1, ' ');
                text_heard += "\n" + bot_name + chat_symb;
                fprintf(stdout, "%s%s%s", "\033[1m", text_heard.c_str(), "\033[0m");
                fflush(stdout);

                embd = ::llama_tokenize(ctx_llama, text_heard, false);

                // Append the new input tokens to the session_tokens vector
                if (!path_session.empty()) {
                    session_tokens.insert(session_tokens.end(), tokens.begin(), tokens.end());
                }

                // text inference
                bool done = false;
                std::string text_to_speak;
                while (true) {
                    // predict
                    if (embd.size() > 0) {
                        if (n_past + (int) embd.size() > n_ctx) {
                            n_past = n_keep;

                            // insert n_left/2 tokens at the start of embd from last_n_tokens
                            embd.insert(embd.begin(), embd_inp.begin() + embd_inp.size() - n_prev, embd_inp.end());
                            // stop saving session if we run out of context
                            path_session = "";
                            //printf("\n---\n");
                            //printf("resetting: '");
                            //for (int i = 0; i < (int) embd.size(); i++) {
                            //    printf("%s", llama_token_to_str(ctx_llama, embd[i]));
                            //}
                            //printf("'\n");
                            //printf("\n---\n");
                        }

                        // try to reuse a matching prefix from the loaded session instead of re-eval (via n_past)
                        // REVIEW
                        if (n_session_consumed < (int) session_tokens.size()) {
                            size_t i = 0;
                            for ( ; i < embd.size(); i++) {
                                if (embd[i] != session_tokens[n_session_consumed]) {
                                    session_tokens.resize(n_session_consumed);
                                    break;
                                }

                                n_past++;
                                n_session_consumed++;

                                if (n_session_consumed >= (int) session_tokens.size()) {
                                    i++;
                                    break;
                                }
                            }
                            if (i > 0) {
                                embd.erase(embd.begin(), embd.begin() + i);
                            }
                        }

                        if (embd.size() > 0 && !path_session.empty()) {
                            session_tokens.insert(session_tokens.end(), embd.begin(), embd.end());
                            n_session_consumed = session_tokens.size();
                        }

                        if (llama_eval(ctx_llama, embd.data(), embd.size(), n_past, params.n_threads)) {
                            fprintf(stderr, "%s : failed to eval\n", __func__);
                            return 1;
                        }
                    }


                    embd_inp.insert(embd_inp.end(), embd.begin(), embd.end());
                    n_past += embd.size();

                    embd.clear();

                    if (done) break;

                    {
                        // out of user input, sample next token
                        const float top_k          = 5;
                        const float top_p          = 0.80f;
                        const float temp           = 0.30f;
                        const float repeat_penalty = 1.1764f;

                        const int repeat_last_n    = 256;

                        if (!path_session.empty() && need_to_save_session) {
                            need_to_save_session = false;
                            llama_save_session_file(ctx_llama, path_session.c_str(), session_tokens.data(), session_tokens.size());
                        }

                        llama_token id = 0;

                        {
                            auto logits = llama_get_logits(ctx_llama);
                            auto n_vocab = llama_n_vocab(ctx_llama);

                            logits[llama_token_eos()] = 0;

                            std::vector<llama_token_data> candidates;
                            candidates.reserve(n_vocab);
                            for (llama_token token_id = 0; token_id < n_vocab; token_id++) {
                                candidates.emplace_back(llama_token_data{token_id, logits[token_id], 0.0f});
                            }

                            llama_token_data_array candidates_p = { candidates.data(), candidates.size(), false };

                            // apply repeat penalty
                            const float nl_logit = logits[llama_token_nl()];

                            llama_sample_repetition_penalty(ctx_llama, &candidates_p,
                                    embd_inp.data() + std::max(0, n_past - repeat_last_n),
                                    repeat_last_n, repeat_penalty);

                            logits[llama_token_nl()] = nl_logit;

                            if (temp <= 0) {
                                // Greedy sampling
                                id = llama_sample_token_greedy(ctx_llama, &candidates_p);
                            } else {
                                // Temperature sampling
                                llama_sample_top_k(ctx_llama, &candidates_p, top_k, 1);
                                llama_sample_top_p(ctx_llama, &candidates_p, top_p, 1);
                                llama_sample_temperature(ctx_llama, &candidates_p, temp);
                                id = llama_sample_token(ctx_llama, &candidates_p);
                            }
                        }

                        if (id != llama_token_eos()) {
                            // add it to the context
                            embd.push_back(id);

                            text_to_speak += llama_token_to_str(ctx_llama, id);

                            printf("%s", llama_token_to_str(ctx_llama, id));
                        }
                    }

                    {
                        std::string last_output;
                        for (int i = embd_inp.size() - 16; i < (int) embd_inp.size(); i++) {
                            last_output += llama_token_to_str(ctx_llama, embd_inp[i]);
                        }
                        last_output += llama_token_to_str(ctx_llama, embd[0]);

                        for (std::string & antiprompt : antiprompts) {
                            if (last_output.find(antiprompt.c_str(), last_output.length() - antiprompt.length(), antiprompt.length()) != std::string::npos) {
                                done = true;
                                text_to_speak = ::replace(text_to_speak, antiprompt, "");
                                fflush(stdout);
                                need_to_save_session = true;
                                break;
                            }
                        }
                    }

                    is_running = sdl_poll_events();

                    if (!is_running) {
                        break;
                    }
                }

                text_to_speak = ::replace(text_to_speak, "\"", "");
                system((params.speak + " " + std::to_string(voice_id) + " \"" + text_to_speak + "\"").c_str());

                audio.clear();

                ++n_iter;
            }
        }
    }

    audio.pause();

    whisper_print_timings(ctx_wsp);
    whisper_free(ctx_wsp);

    llama_print_timings(ctx_llama);
    llama_free(ctx_llama);

    return 0;
}
