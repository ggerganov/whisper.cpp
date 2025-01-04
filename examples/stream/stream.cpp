// Real-time speech recognition of input from a microphone
//
// A very quick-n-dirty implementation serving mainly as a proof of concept.
//
#include "common-sdl.h"
#include "common.h"
#include "whisper.h"

#include <cassert>
#include <codecvt>
#include <cstdio>
#include <deque>
#include <locale>
#include <string>
#include <thread>
#include <vector>
#include <fstream>
#include <fcntl.h>

#ifdef _WIN32
    #define NOMINMAX
    #include <windows.h>
    #include <io.h>
#else
    #include <unistd.h>
#endif

void setStdinNonBlocking() {
#ifdef _WIN32
    DWORD mode;
    HANDLE stdinHandle = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(stdinHandle, &mode);
    mode &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
    SetConsoleMode(stdinHandle, mode);
#else
    fcntl(fileno(stdin), F_SETFL, fcntl(fileno(stdin), F_GETFL, 0) | O_NONBLOCK);
#endif
}

void setStdinBlocking() {
#ifdef _WIN32
    DWORD mode;
    HANDLE stdinHandle = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(stdinHandle, &mode);
    mode |= ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT;
    SetConsoleMode(stdinHandle, mode);
#else
    fcntl(fileno(stdin), F_SETFL, fcntl(fileno(stdin), F_GETFL, 0) & ~O_NONBLOCK);
#endif
}


// command-line parameters
struct whisper_params {
    int32_t n_threads  = std::min(4, (int32_t) std::thread::hardware_concurrency());
    int32_t step_ms    = 3000;
    int32_t length_ms  = 10000;
    int32_t keep_ms    = 200;
    int32_t capture_id = -1;
    int32_t max_tokens = 128;
    int32_t audio_ctx  = 0;

    float vad_thold    = 0.6f;
    float freq_thold   = 100.0f;

    bool translate     = false;
    bool no_fallback   = false;
    bool print_special = false;
    bool no_context    = true;
    bool no_timestamps = false;
    bool tinydiarize   = false;
    bool save_audio    = false; // save audio to wav file
    bool use_gpu       = true;
    bool flash_attn    = false;
    bool interim       = false;
    bool delete_vt100  = true;
    bool test_pipe     = false;

    std::string language  = "en";
    std::string model     = "models/ggml-base.en.bin";
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
        else if (arg == "-t"    || arg == "--threads")       { params.n_threads     = std::stoi(argv[++i]); }
        else if (                  arg == "--step")          { params.step_ms       = std::stoi(argv[++i]); }
        else if (                  arg == "--length")        { params.length_ms     = std::stoi(argv[++i]); }
        else if (                  arg == "--keep")          { params.keep_ms       = std::stoi(argv[++i]); }
        else if (arg == "-c"    || arg == "--capture")       { params.capture_id    = std::stoi(argv[++i]); }
        else if (arg == "-mt"   || arg == "--max-tokens")    { params.max_tokens    = std::stoi(argv[++i]); }
        else if (arg == "-ac"   || arg == "--audio-ctx")     { params.audio_ctx     = std::stoi(argv[++i]); }
        else if (arg == "-vth"  || arg == "--vad-thold")     { params.vad_thold     = std::stof(argv[++i]); }
        else if (arg == "-fth"  || arg == "--freq-thold")    { params.freq_thold    = std::stof(argv[++i]); }
        else if (arg == "-tr"   || arg == "--translate")     { params.translate     = true; }
        else if (arg == "-nf"   || arg == "--no-fallback")   { params.no_fallback   = true; }
        else if (arg == "-ps"   || arg == "--print-special") { params.print_special = true; }
        else if (arg == "-kc"   || arg == "--keep-context")  { params.no_context    = false; }
        else if (arg == "-nt"   || arg == "--no-timestamps") { params.no_timestamps = true; }
        else if (arg == "-l"    || arg == "--language")      { params.language      = argv[++i]; }
        else if (arg == "-m"    || arg == "--model")         { params.model         = argv[++i]; }
        else if (arg == "-f"    || arg == "--file")          { params.fname_out     = argv[++i]; }
        else if (arg == "-tdrz" || arg == "--tinydiarize")   { params.tinydiarize   = true; }
        else if (arg == "-sa"   || arg == "--save-audio")    { params.save_audio    = true; }
        else if (arg == "-ng"   || arg == "--no-gpu")        { params.use_gpu       = false; }
        else if (arg == "-fa"   || arg == "--flash-attn")    { params.flash_attn    = true; }
        else if (arg == "-int"  || arg == "--interim")       { params.interim       = true; }
        else if (arg == "-nvt"  || arg == "--no-vt100")      { params.delete_vt100  = false; }
        else if (                  arg == "--test-pipe")     { params.test_pipe     = true; }

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
    fprintf(stderr, "  -t N,     --threads N     [%-7d] number of threads to use during computation\n",    params.n_threads);
    fprintf(stderr, "            --step N        [%-7d] audio step size in milliseconds\n",                params.step_ms);
    fprintf(stderr, "            --length N      [%-7d] audio length in milliseconds\n",                   params.length_ms);
    fprintf(stderr, "            --keep N        [%-7d] audio to keep from previous step in ms\n",         params.keep_ms);
    fprintf(stderr, "  -c ID,    --capture ID    [%-7d] capture device ID\n",                              params.capture_id);
    fprintf(stderr, "  -mt N,    --max-tokens N  [%-7d] maximum number of tokens per audio chunk\n",       params.max_tokens);
    fprintf(stderr, "  -ac N,    --audio-ctx N   [%-7d] audio context size (0 - all)\n",                   params.audio_ctx);
    fprintf(stderr, "  -vth N,   --vad-thold N   [%-7.2f] voice activity detection threshold\n",           params.vad_thold);
    fprintf(stderr, "  -fth N,   --freq-thold N  [%-7.2f] high-pass frequency cutoff\n",                   params.freq_thold);
    fprintf(stderr, "  -tr,      --translate     [%-7s] translate from source language to english\n",      params.translate ? "true" : "false");
    fprintf(stderr, "  -nf,      --no-fallback   [%-7s] do not use temperature fallback while decoding\n", params.no_fallback ? "true" : "false");
    fprintf(stderr, "  -ps,      --print-special [%-7s] print special tokens\n",                           params.print_special ? "true" : "false");
    fprintf(stderr, "  -kc,      --keep-context  [%-7s] keep context between audio chunks\n",              params.no_context ? "false" : "true");
    fprintf(stderr, "  -nt,      --no-timestamps [%-7s] do not print timestamps\n",                        params.no_timestamps ? "true" : "false");
    fprintf(stderr, "  -l LANG,  --language LANG [%-7s] spoken language\n",                                params.language.c_str());
    fprintf(stderr, "  -m FNAME, --model FNAME   [%-7s] model path\n",                                     params.model.c_str());
    fprintf(stderr, "  -f FNAME, --file FNAME    [%-7s] text output file name\n",                          params.fname_out.c_str());
    fprintf(stderr, "  -tdrz,    --tinydiarize   [%-7s] enable tinydiarize (requires a tdrz model)\n",     params.tinydiarize ? "true" : "false");
    fprintf(stderr, "  -sa,      --save-audio    [%-7s] save the recorded audio to a file\n",              params.save_audio ? "true" : "false");
    fprintf(stderr, "  -ng,      --no-gpu        [%-7s] disable GPU inference\n",                          params.use_gpu ? "false" : "true");
    fprintf(stderr, "  -fa,      --flash-attn    [%-7s] flash attention during inference\n",               params.flash_attn ? "true" : "false");
    fprintf(stderr, "  -int,     --interim       [%-7s] show interim report in vad every step\n",          params.interim ? "true" : "false");
    fprintf(stderr, "  -nvt,     --no-vt100      [%-7s] do not delete unconfirmed result\n",               params.delete_vt100 ? "false" : "true");
    fprintf(stderr, "            --test-pipe     [%-7s] use all data from pipe\n",                         params.test_pipe ? "true" : "false");
    fprintf(stderr, "\n");
}

int main(int argc, char ** argv) {
    whisper_params params;

    if (whisper_params_parse(argc, argv, params) == false) {
        return 1;
    }

    params.keep_ms   = std::min(params.keep_ms,   abs(params.step_ms));
    params.length_ms = std::max(params.length_ms, abs(params.step_ms));

    const int n_samples_step = (1e-3*abs(params.step_ms))*WHISPER_SAMPLE_RATE;
    const int n_samples_len  = (1e-3*params.length_ms   )*WHISPER_SAMPLE_RATE;
    const int n_samples_keep = (1e-3*params.keep_ms     )*WHISPER_SAMPLE_RATE;
    const int n_samples_30s  = (1e-3*30000.0            )*WHISPER_SAMPLE_RATE;
    const int n_samples_100ms= (1e-3*100.0              )*WHISPER_SAMPLE_RATE;

    const bool use_vad = params.step_ms <= 0; // sliding window mode uses VAD

    // init audio

    audio_async audio(params.length_ms);
    bool piped = !isatty(fileno(stdin));

    if (piped) {
        #ifdef _WIN32
        _setmode(_fileno(stdin), _O_BINARY);
        #else
        freopen(NULL, "rb", stdin);
        #endif
    } else {
        if (!audio.init(params.capture_id, WHISPER_SAMPLE_RATE)) {
            fprintf(stderr, "%s: audio.init() failed!\n", __func__);
            return 1;
        }

        audio.resume();
    }

    // whisper init
    if (params.language != "auto" && whisper_lang_id(params.language.c_str()) == -1){
        fprintf(stderr, "error: unknown language '%s'\n", params.language.c_str());
        whisper_print_usage(argc, argv, params);
        exit(0);
    }

    struct whisper_context_params cparams = whisper_context_default_params();

    cparams.use_gpu    = params.use_gpu;
    cparams.flash_attn = params.flash_attn;

    struct whisper_context * ctx = whisper_init_from_file_with_params(params.model.c_str(), cparams);

    std::vector<float> pcmf32(n_samples_30s, 0.0f);
    std::deque<float> pcmf32_deque;
    int n_samples_new = 0;
    int n_samples_old = 0;

    std::vector<whisper_token> prompt_tokens;

    // print some info about the processing
    {
        fprintf(stderr, "\n");
        if (!whisper_is_multilingual(ctx)) {
            if (params.language != "en" || params.translate) {
                params.language = "en";
                params.translate = false;
                fprintf(stderr, "%s: WARNING: model is not multilingual, ignoring language and translation options\n", __func__);
            }
        }
        fprintf(stderr, "%s: processing %d samples (step = %.1f sec / len = %.1f sec / keep = %.1f sec), %d threads, lang = %s, task = %s, timestamps = %d ...\n",
                __func__,
                n_samples_step,
                float(n_samples_step)/WHISPER_SAMPLE_RATE,
                float(n_samples_len )/WHISPER_SAMPLE_RATE,
                float(n_samples_keep)/WHISPER_SAMPLE_RATE,
                params.n_threads,
                params.language.c_str(),
                params.translate ? "translate" : "transcribe",
                params.no_timestamps ? 0 : 1);

        if (!use_vad) {
            fprintf(stderr, "%s: no_context = %d\n", __func__, params.no_context);
        } else {
            fprintf(stderr, "%s: using VAD, will transcribe on speech activity\n", __func__);
        }

        fprintf(stderr, "\n");
    }

    int n_iter = 0;

    bool is_running = true;

    std::ofstream fout;
    if (params.fname_out.length() > 0) {
        fout.open(params.fname_out);
        if (!fout.is_open()) {
            fprintf(stderr, "%s: failed to open output file '%s'!\n", __func__, params.fname_out.c_str());
            return 1;
        }
    }

    wav_writer wavWriter;
    // save wav file
    if (params.save_audio) {
        // Get current date/time for filename
        time_t now = time(0);
        char buffer[80];
        strftime(buffer, sizeof(buffer), "%Y%m%d%H%M%S", localtime(&now));
        std::string filename = std::string(buffer) + ".wav";

        wavWriter.open(filename, WHISPER_SAMPLE_RATE, 16, 1);
    }

    // ignore premature stdin
    int n_mod = 0;
    if (piped && !params.test_pipe) {
        const auto n_bytes_len = sizeof(float) * n_samples_len;
        setStdinNonBlocking();
        while (true) {
            const auto n_bytes_read = read(fileno(stdin), pcmf32.data(), n_bytes_len);
            if (n_bytes_read == -1 && errno == EAGAIN) {
                break;
            } else if (n_bytes_read < 1) {
                fprintf(stderr, "stdin ended too early\n");
                is_running = false;
                break;
            }
            n_mod = n_bytes_read % sizeof(float);
            if (n_bytes_read < n_bytes_len) {
                break;
            }
        }
    }

    fprintf(stderr, "[Start speaking]\n");
    fflush(stderr);

    if (piped) {
        // ignore the partial sample
        if (n_mod > 0) {
            const auto n_remain = sizeof(float) - n_mod;
            setStdinBlocking();
            if (n_remain != fread(pcmf32.data(), 1, n_remain, stdin)) {
                is_running = false;
            }
        }
        setStdinNonBlocking();
    }

    auto t_last  = std::chrono::high_resolution_clock::now();
    auto t_interim = t_last;
    bool is_interim = false;
    const auto t_start = t_last;
    std::string s_to_delete = "";

    // main audio loop
    while (is_running) {
        // handle Ctrl + C
        is_running = sdl_poll_events();

        if (!is_running) {
            break;
        }

        // process new audio
        const auto t_now  = std::chrono::high_resolution_clock::now();
        const auto t_diff = std::chrono::duration_cast<std::chrono::milliseconds>(t_now - t_last).count();

        // get new audio
        if (n_samples_new > n_samples_step) {
            pcmf32.clear();
        } else if (piped) {
            pcmf32.resize(n_samples_len);
            char *p_buf = (char *)pcmf32.data();
            const auto n_bytes_min = (n_samples_step - n_samples_new) * sizeof(float);
            auto n_bytes_wanted = n_samples_len * sizeof(float);
            auto n_bytes_read = 0;
            while (n_bytes_wanted > 0) {
                const auto n_read = read(fileno(stdin), p_buf + n_bytes_read, n_bytes_wanted);
                if (n_read == 0 || n_read == -1 && errno != EAGAIN) {
                    fprintf(stderr, "read(stdin) returned %zd, errno = %d\n", n_read, errno);
                    is_running = false;
                    break;
                }
                n_bytes_read += std::max<long>(0, n_read);
                if (n_bytes_read < n_bytes_min) {
                    n_bytes_wanted = n_bytes_min - n_bytes_read;
                } else {
                    n_bytes_wanted = n_bytes_read % sizeof(float);
                }
                if (n_bytes_wanted > 0) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
            pcmf32.resize(n_bytes_read / sizeof(float));
        } else if (t_diff < abs(params.step_ms)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(abs(params.step_ms) - t_diff));
            continue;
        } else {
            audio.next(pcmf32);
        }

        int n_samples_buf = pcmf32.size();

        if (params.save_audio && n_samples_buf > 0) {
            wavWriter.write(pcmf32.data(), n_samples_buf);
        }

        copy(pcmf32.begin(), pcmf32.end(), std::back_inserter(pcmf32_deque));
        if (pcmf32_deque.size() > n_samples_30s) {
            pcmf32_deque.erase(pcmf32_deque.begin(), pcmf32_deque.end() - n_samples_30s);
        }

        n_samples_new += n_samples_buf;
        if (!use_vad && n_samples_new > 2*n_samples_step) {
            fprintf(stderr, "\n\n%s: WARNING: cannot process audio fast enough, dropping audio ...\n", __func__);
            fprintf(stderr, "t_diff = %.2fs, new = %.2fs, buf = %.2fs\n\n", 1e-3*t_diff, float(n_samples_new)/WHISPER_SAMPLE_RATE, float(n_samples_buf)/WHISPER_SAMPLE_RATE);
            n_samples_old = 0;
            n_samples_new = 0;
            t_last = t_now;
            continue;
        }

        if (n_samples_old + n_samples_new == 0) {
            continue;
        }

        is_interim = false;
        bool is_aborted = true;

        n_samples_buf = std::min(n_samples_len, n_samples_old + n_samples_new);
        pcmf32.resize(n_samples_buf);
        copy(pcmf32_deque.end() - n_samples_buf, pcmf32_deque.end(), pcmf32.begin());

        if (!use_vad){
            n_samples_old += n_samples_new;
            n_samples_new = 0;

            t_last = t_now;
        } else {
            is_aborted = (n_samples_buf > n_samples_len);

            if (!is_running || is_aborted || ::vad_simple(pcmf32, WHISPER_SAMPLE_RATE, std::min(1000, abs(params.step_ms) / 2), params.vad_thold, params.freq_thold, false)) {
                n_samples_new = 0;
                n_samples_old = 0;

                t_last = t_now;
            } else {
                const auto n_interim_diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_now - t_interim).count();

                if (params.interim && n_interim_diff_ms > abs(params.step_ms)) {
                    is_interim = (n_interim_diff_ms < params.length_ms - abs(params.step_ms));
                    n_samples_old += n_samples_new;
                    n_samples_new = 0;
                    pcmf32.resize(n_samples_old);
                    copy(pcmf32_deque.end() - n_samples_old, pcmf32_deque.end(), pcmf32.begin());
                } else {
                    n_samples_new -= n_samples_100ms;
                    n_samples_old = std::min(n_samples_len, n_samples_old + n_samples_100ms);
                    if (!piped) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                    continue;
                }
            }
        }

        // run the inference
        {
            whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);

            wparams.print_progress   = false;
            wparams.print_special    = params.print_special;
            wparams.print_realtime   = false;
            wparams.print_timestamps = !params.no_timestamps;
            wparams.translate        = params.translate;
            wparams.single_segment   = !use_vad;
            wparams.max_tokens       = params.max_tokens;
            wparams.language         = params.language.c_str();
            wparams.n_threads        = params.n_threads;

            wparams.audio_ctx        = params.audio_ctx;

            wparams.tdrz_enable      = params.tinydiarize; // [TDRZ]

            // disable temperature fallback
            //wparams.temperature_inc  = -1.0f;
            wparams.temperature_inc  = params.no_fallback ? 0.0f : wparams.temperature_inc;

            wparams.prompt_tokens    = params.no_context ? nullptr : prompt_tokens.data();
            wparams.prompt_n_tokens  = params.no_context ? 0       : prompt_tokens.size();

            {
                auto pcm_size = pcmf32.size();
                if (pcm_size < WHISPER_SAMPLE_RATE * 1.1) {
                    pcmf32.resize(pcm_size + WHISPER_SAMPLE_RATE, 0.0f);
                }
                if (whisper_full(ctx, wparams, pcmf32.data(), pcmf32.size()) != 0) {
                    fprintf(stderr, "%s: failed to process audio\n", argv[0]);
                    return 6;
                }
                pcmf32.resize(pcm_size);
            }
            t_interim  = std::chrono::high_resolution_clock::now();

            // print result;
            int n_segments;
            bool no_confirmed = (!use_vad && n_samples_old < n_samples_len - n_samples_step);
            std::ostringstream text;
            {
                if (params.delete_vt100 && s_to_delete.size()) {
                    printf("\33[2K\r");

                    // print long empty line to clear the previous line
                    printf("%s", std::string(s_to_delete.size(), ' ').c_str());

                    printf("\33[2K\r");
                }
                s_to_delete.clear();

                n_segments = whisper_full_n_segments(ctx);
                no_confirmed = (no_confirmed || is_interim && n_segments <= 1);
                if (is_running && is_interim && !no_confirmed) {
                    const int64_t t1_ms = whisper_full_get_segment_t1(ctx, n_segments - 2) * 10;
                    if (t1_ms < abs(params.step_ms)) {
                        // too short to confirm
                        no_confirmed = true;
                    } else {
                        t_last += std::chrono::milliseconds(t1_ms);
                        const auto n_samples_confirmed = (1e-3*t1_ms)*WHISPER_SAMPLE_RATE;
                        pcmf32.resize(n_samples_confirmed); // for timestamps
                        n_samples_old -= n_samples_confirmed;
                    }
                }

                if (use_vad && !params.no_timestamps && (!is_running || !no_confirmed)) {
                    const int64_t t1 = (t_last - t_start).count()/1000000;
                    const int64_t t0 = std::max(0.0, t1 - pcmf32.size()*1000.0/WHISPER_SAMPLE_RATE);

                    text << std::endl;
                    text << "### Transcription " << n_iter << " START | t0 = " << t0 << " ms | t1 = " << t1 << " ms" << std::endl;
                    text << std::endl;
                }

                for (int i = 0; i < n_segments; ++i) {
                    std::string i_text = whisper_full_get_segment_text(ctx, i);

                    // last segment may be s_to_delete
                    if (i == n_segments - 1 && is_running && (no_confirmed || is_interim)) {
                        if (params.no_timestamps && i > 0) {
                            text << std::endl;
                        }
                        if (is_interim) {
                            // utf-8 cannot be simply cut into two
                            std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
                            const auto t_u32 = conv.from_bytes(i_text);
                            const auto t_sub = conv.to_bytes(t_u32.substr(0, t_u32.size() * 0.7));
                            i_text = t_sub + "…";
                        }
                        if (s_to_delete.size() > 0) {
                            s_to_delete += " ";
                        }
                        s_to_delete += i_text;
                        if (!params.delete_vt100) {
                            s_to_delete = "(" + s_to_delete + ")";
                        }
                        break;
                    }

                    if (is_running && no_confirmed) {
                        if (s_to_delete.size() > 0) {
                            s_to_delete += " ";
                        }
                        s_to_delete += i_text;
                    } else if (params.no_timestamps) {
                        if (i > 0) {
                            text << std::endl;
                        }
                        text << i_text;
                    } else if (!is_running || !(is_interim && i == n_segments - 1)) {
                        const int64_t t_end = (t_last - t_start).count()/1000000;
                        const int64_t t_beg = std::max(0.0, t_end - pcmf32.size()*1000.0/WHISPER_SAMPLE_RATE);
                        const int64_t t0 = t_beg/10 + whisper_full_get_segment_t0(ctx, i);
                        const int64_t t1 = t_beg/10 + whisper_full_get_segment_t1(ctx, i);

                        text << "[" << to_timestamp(t0, false) << " --> " << to_timestamp(t1, false) << "]  " << i_text;

                        if (whisper_full_get_segment_speaker_turn_next(ctx, i)) {
                            text << " [SPEAKER_TURN]";
                        }

                        text << std::endl;
                    }
                }

                if (use_vad && !params.no_timestamps && (!is_running || !no_confirmed)) {
                    text << std::endl;
                    text << "### Transcription " << n_iter << " END";
                    text << std::endl;
                    if (s_to_delete.size() > 0) {
                        text << std::endl;
                    }
                }
            }

            if (params.fname_out.length() > 0) {
                fout << text.str();
                fout << std::endl;
            }

            if (!no_confirmed) {
                ++n_iter;
            }

            printf("%s", text.str().c_str());

            if (is_running && (no_confirmed || is_interim)) {
                printf("%s%s", s_to_delete.c_str(), params.delete_vt100 ? "" : "\n");
                --n_segments; // exclude s_to_delete from context
            } else {
                printf("\n");
                s_to_delete = "";

                if (is_aborted) {
                    // keep part of the audio for next iteration to try to mitigate word boundary issues
                    n_samples_old = std::min(n_samples_old, n_samples_keep);
                }
            }

            // Add tokens of the last full length segment as the prompt
            if (!no_confirmed && !params.no_context) {
                prompt_tokens.clear();

                for (int i = 0; i < n_segments; ++i) {
                    const int token_count = whisper_full_n_tokens(ctx, i);
                    for (int j = 0; j < token_count; ++j) {
                        prompt_tokens.push_back(whisper_full_get_token_id(ctx, i, j));
                    }
                }
            }
            fflush(stdout);
        }
    }

    audio.pause();

    whisper_print_timings(ctx);
    whisper_free(ctx);

    return 0;
}
