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


// command-line parameters
struct whisper_params {
    int32_t n_threads  = std::min(4, (int32_t) std::thread::hardware_concurrency());
    int32_t step_ms    = 3000;
    int32_t length_ms  = 10000;
    int32_t keep_ms    = 200;
    int32_t capture_id = -1;
    int32_t max_tokens = 128;
    int32_t audio_ctx  = 0;
    int32_t n_tmp_segs = 1;

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
    fprintf(stderr, "\n");
}

int main(int argc, char ** argv) {
    whisper_params params;

    if (whisper_params_parse(argc, argv, params) == false) {
        return 1;
    }

    if (params.step_ms == 0) {
        params.step_ms = -2000; // reasonable default for VAD
    }
    params.keep_ms   = std::min(params.keep_ms,   abs(params.step_ms));
    params.length_ms = std::max(params.length_ms, abs(params.step_ms));

    const int n_samples_step = (1e-3*abs(params.step_ms))*WHISPER_SAMPLE_RATE;
    const int n_samples_len  = (1e-3*params.length_ms   )*WHISPER_SAMPLE_RATE;
    const int n_samples_keep = (1e-3*params.keep_ms     )*WHISPER_SAMPLE_RATE;
    const int n_samples_30s  = (1e-3*30000.0            )*WHISPER_SAMPLE_RATE;
    const int n_samples_100ms= (1e-3*100.0              )*WHISPER_SAMPLE_RATE;

    const bool use_vad = params.step_ms <= 0; // sliding window mode uses VAD
    const bool piped = !isatty(fileno(stdin));

    // init audio

    audio_async audio(params.length_ms);

    if (!piped) {
        if (!audio.init(params.capture_id, WHISPER_SAMPLE_RATE)) {
            fprintf(stderr, "%s: audio.init() failed!\n", __func__);
            return 1;
        }

        audio.resume();
    } else {
        fprintf(stderr, "%s: audio is from stdin, not from microphone\n", __func__);

        #ifdef _WIN32
        _setmode(_fileno(stdin), _O_BINARY);
        #else
        freopen(NULL, "rb", stdin);
        #endif

        // non-blocking mode
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

        fprintf(stderr, "%s: no_context = %d\n", __func__, params.no_context);
        if (use_vad) {
            fprintf(stderr, "%s: using VAD, will transcribe on speech activity\n", __func__);
            fprintf(stderr, "%s: interim report = %d, temporary segments = %d\n", __func__, params.interim, params.n_tmp_segs);
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
    fprintf(stderr, "[Start speaking]\n");
    fflush(stderr);

    auto t_last  = std::chrono::high_resolution_clock::now();
    const auto t_start = t_last;
    auto t_interim = t_last;
    std::string s_tmp = "";

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

        if (!piped) {
            const auto sleep_ms = abs(params.step_ms) - t_diff;
            if (sleep_ms > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
                continue;
            }

            audio.next(pcmf32);

            if ((int) pcmf32.size() > 2*n_samples_step) {
                fprintf(stderr, "\n\n%s: WARNING: cannot process audio fast enough, dropping audio ...\n", __func__);
                fprintf(stderr, "t_diff = %.2f sec, prev = %.2f sec, got = %.2f sec\n\n", 1e-3*t_diff, float(n_samples_new)/WHISPER_SAMPLE_RATE, float(pcmf32.size())/WHISPER_SAMPLE_RATE);
                n_samples_old = 0;
                n_samples_new = 0;
                t_last = t_now;
                continue;
            }
        } else {
            // piped: need at least step_ms but try to get length_ms at first
            const auto n_bytes_min = std::max<long>(0, (n_samples_step - n_samples_new) * sizeof(float));
            auto n_bytes_wanted = n_samples_len * sizeof(float);
            pcmf32.resize(n_samples_len);

            long n_bytes_read = 0;
            while (n_bytes_wanted > 0) {
                char *p_buf = (char *)pcmf32.data();
                const auto n_read = read(fileno(stdin), p_buf + n_bytes_read, n_bytes_wanted);
                if (n_read == 0 || n_read == -1 && errno != EAGAIN) {
                    fprintf(stderr, "read(stdin) returned %zd, errno = %s\n", n_read, strerror(errno));
                    is_running = false; // flush all results
                    break;
                }
                n_bytes_read += std::max<long>(0, n_read);
                if (n_bytes_read < n_bytes_min) {
                    n_bytes_wanted = n_bytes_min - n_bytes_read;
                } else {
                    const auto n_mod = n_bytes_read % sizeof(float);
                    n_bytes_wanted = (n_mod != 0) ? sizeof(float) - n_mod : 0;
                }
                const auto est_sleep_ms = 1000 * n_bytes_wanted / sizeof(float) / WHISPER_SAMPLE_RATE;
                std::this_thread::sleep_for(std::chrono::milliseconds(est_sleep_ms));
            }
            pcmf32.resize(n_bytes_read / sizeof(float));
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

        if (n_samples_old + n_samples_new == 0) {
            continue;
        }

        // prepare pcmf32 for inference
        n_samples_buf = n_samples_old + n_samples_new;
        pcmf32.resize(n_samples_buf);
        copy(pcmf32_deque.end() - n_samples_buf, pcmf32_deque.end(), pcmf32.begin());

        // chop the audio unconditionally
        bool use_keep_ms = ((!use_vad || params.interim) && n_samples_buf > n_samples_len);

        // interim report in vad mode: once every step_ms,
        // run the inference even if vad returns false,
        // confirm (n_segments - params.n_tmp_segs) segments,
        // and print other segments as s_tmp, which will be deleted
        bool is_interim = false;

        if (!use_vad || use_keep_ms || !is_running) {
            use_keep_ms = true;
            n_samples_old += n_samples_new;
            n_samples_new = 0;

            t_last = t_now;
        } else {
            if (::vad_simple(pcmf32, WHISPER_SAMPLE_RATE, std::min(1000, abs(params.step_ms) / 2), params.vad_thold, params.freq_thold, false)) {
                n_samples_new = 0;
                n_samples_old = 0;

                t_last = t_now;
            } else {
                const auto interim_diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_now - t_interim).count();

                if (params.interim && interim_diff_ms > abs(params.step_ms)) {
                    is_interim = true;
                    n_samples_old += n_samples_new;
                    n_samples_new = 0;
                } else {
                    // sliding window
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

            // call whisper_full() with at least 1 sec of buffer
            {
                auto pcm_size = pcmf32.size();
                if (pcm_size < WHISPER_SAMPLE_RATE * 1.1) {
                    pcmf32.resize(pcm_size + WHISPER_SAMPLE_RATE, 0.0f);
                }
                if (whisper_full(ctx, wparams, pcmf32.data(), pcm_size) != 0) {
                    fprintf(stderr, "%s: failed to process audio\n", argv[0]);
                    return 6;
                }
                pcmf32.resize(pcm_size);
            }
            t_interim = std::chrono::high_resolution_clock::now();

            // print result;
            int n_segments;
            bool is_all_tmp = (!use_vad && n_samples_old < n_samples_len - n_samples_step);
            std::ostringstream ss_output;

            {
                if (params.delete_vt100 && s_tmp.size()) {
                    printf("\33[2K\r");

                    // print long empty line to clear the previous line
                    printf("%s", std::string(s_tmp.size(), ' ').c_str());

                    printf("\33[2K\r");
                }
                s_tmp.clear();

                n_segments = whisper_full_n_segments(ctx);
                is_all_tmp = (is_running && (is_all_tmp || is_interim && n_segments <= params.n_tmp_segs));
                if (is_running && is_interim && !is_all_tmp) {
                    const int64_t t1_ms = whisper_full_get_segment_t1(ctx, n_segments - params.n_tmp_segs - 1) * 10;
                    if (t1_ms < abs(params.step_ms)) {
                        // too short to confirm
                        is_all_tmp = true;
                    } else {
                        t_last += std::chrono::milliseconds(t1_ms);
                        const auto n_samples_confirmed = (1e-3*t1_ms)*WHISPER_SAMPLE_RATE;
                        pcmf32.resize(n_samples_confirmed); // for timestamps
                        n_samples_old -= n_samples_confirmed; // kept for next iteration
                    }
                }

                bool show_n_iter = (use_vad && !params.no_timestamps && !is_all_tmp);

                if (show_n_iter) {
                    const int64_t t1 = (t_last - t_start).count()/1000000;
                    const int64_t t0 = std::max(0.0, t1 - pcmf32.size()*1000.0/WHISPER_SAMPLE_RATE);

                    ss_output << std::endl;
                    ss_output << "### Transcription " << n_iter << " START | t0 = " << t0 << " ms | t1 = " << t1 << " ms" << std::endl;
                    ss_output << std::endl;
                }

                for (int i = 0; i < n_segments; ++i) {
                    std::string text = whisper_full_get_segment_text(ctx, i);

                    // last segment(s) may be s_tmp
                    if (i >= n_segments - params.n_tmp_segs && is_running && (is_all_tmp || is_interim)) {
                        if (params.no_timestamps && i > 0) {
                            ss_output << std::endl;
                        }
                        if (is_interim) {
                            // utf-8 cannot be simply cut, so use char32_t
                            std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
                            const auto s_u32 = conv.from_bytes(text);
                            const auto s_sub = conv.to_bytes(s_u32.substr(0, s_u32.size() * 0.9));
                            text = s_sub + "…";
                        }
                        if (s_tmp.size() > 0) {
                            s_tmp += " ";
                        }
                        s_tmp += text;
                        continue;
                    }

                    if (is_all_tmp) {
                        if (s_tmp.size() > 0) {
                            s_tmp += " ";
                        }
                        s_tmp += text;
                    } else if (params.no_timestamps) {
                        if (i > 0) {
                            ss_output << std::endl;
                        }
                        ss_output << text;
                    } else {
                        const int64_t t_end = (t_last - t_start).count()/1000000;
                        const int64_t t_beg = std::max(0.0, t_end - pcmf32.size()*1000.0/WHISPER_SAMPLE_RATE);
                        const int64_t t0 = t_beg/10 + whisper_full_get_segment_t0(ctx, i);
                        const int64_t t1 = t_beg/10 + whisper_full_get_segment_t1(ctx, i);

                        ss_output << "[" << to_timestamp(t0, false) << " --> " << to_timestamp(t1, false) << "]  " << text;

                        if (whisper_full_get_segment_speaker_turn_next(ctx, i)) {
                            ss_output << " [SPEAKER_TURN]";
                        }

                        ss_output << std::endl;
                    }
                }

                if (show_n_iter) {
                    ss_output << std::endl;
                    ss_output << "### Transcription " << n_iter << " END" << std::endl;
                    if (s_tmp.size() > 0) {
                        ss_output << std::endl;
                    }
                }
            }

            if (params.fname_out.length() > 0) {
                fout << ss_output.str();
                fout << std::endl;
            }

            if (!is_all_tmp) {
                ++n_iter;
            }

            printf("%s", ss_output.str().c_str());

            if (s_tmp.size() > 0) {
                if (!params.delete_vt100) {
                    s_tmp = "(" + s_tmp + ")\n";
                }
                printf("%s", s_tmp.c_str());

                // exclude s_tmp from context
                n_segments -= is_all_tmp ? n_segments : params.n_tmp_segs;
            } else {
                printf("\n");
                s_tmp = "";

                if (use_keep_ms) {
                    // keep part of the audio for next iteration to try to mitigate word boundary issues
                    n_samples_old = std::min(n_samples_old, n_samples_keep);
                }
            }

            // Add tokens of the last full length segment as the prompt
            if (n_segments > 0 && !params.no_context) {
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
