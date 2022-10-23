#include "whisper.h"

// third-party utilities
// use your favorite implementations
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#include <cmath>
#include <fstream>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

// Terminal color map. 10 colors grouped in ranges [0.0, 0.1, ..., 0.9]
// Lowest is red, middle is yellow, highest is green.
const std::vector<std::string> k_colors = {
    "\033[38;5;196m", "\033[38;5;202m", "\033[38;5;208m", "\033[38;5;214m", "\033[38;5;220m",
    "\033[38;5;226m", "\033[38;5;190m", "\033[38;5;154m", "\033[38;5;118m", "\033[38;5;82m",
};

//  500 -> 00:05.000
// 6000 -> 01:00.000
std::string to_timestamp(int64_t t) {
    int64_t msec = t * 10;
    int64_t hr = msec / (1000 * 60 * 60);
    msec = msec - hr * (1000 * 60 * 60);
    int64_t min = msec / (1000 * 60);
    msec = msec - min * (1000 * 60);
    int64_t sec = msec / 1000;
    msec = msec - sec * 1000;

    char buf[32];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d.%03d", (int) hr, (int) min, (int) sec, (int) msec);

    return std::string(buf);
}

// command-line parameters
struct whisper_params {
    int32_t seed        = -1; // RNG seed, not used currently
    int32_t n_threads   = std::min(4, (int32_t) std::thread::hardware_concurrency());
    int32_t offset_t_ms = 0;
    int32_t offset_n    = 0;

    bool verbose              = false;
    bool translate            = false;
    bool output_txt           = false;
    bool output_vtt           = false;
    bool output_srt           = false;
    bool print_special_tokens = false;
    bool print_colors         = false;
    bool no_timestamps        = false;

    std::string language  = "en";
    std::string model     = "models/ggml-base.en.bin";

    std::vector<std::string> fname_inp = {};
};

void whisper_print_usage(int argc, char ** argv, const whisper_params & params);

bool whisper_params_parse(int argc, char ** argv, whisper_params & params) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg[0] != '-') {
            params.fname_inp.push_back(arg);
            continue;
        }

        if (arg == "-s" || arg == "--seed") {
            params.seed = std::stoi(argv[++i]);
        } else if (arg == "-t" || arg == "--threads") {
            params.n_threads = std::stoi(argv[++i]);
        } else if (arg == "-ot" || arg == "--offset-t") {
            params.offset_t_ms = std::stoi(argv[++i]);
        } else if (arg == "-on" || arg == "--offset-n") {
            params.offset_n = std::stoi(argv[++i]);
        } else if (arg == "-v" || arg == "--verbose") {
            params.verbose = true;
        } else if (arg == "--translate") {
            params.translate = true;
        } else if (arg == "-l" || arg == "--language") {
            params.language = argv[++i];
            if (whisper_lang_id(params.language.c_str()) == -1) {
                fprintf(stderr, "error: unknown language '%s'\n", params.language.c_str());
                whisper_print_usage(argc, argv, params);
                exit(0);
            }
        } else if (arg == "-otxt" || arg == "--output-txt") {
            params.output_txt = true;
        } else if (arg == "-ovtt" || arg == "--output-vtt") {
            params.output_vtt = true;
        } else if (arg == "-osrt" || arg == "--output-srt") {
            params.output_srt = true;
        } else if (arg == "-ps" || arg == "--print_special") {
            params.print_special_tokens = true;
        } else if (arg == "-pc" || arg == "--print_colors") {
            params.print_colors = true;
        } else if (arg == "-nt" || arg == "--no_timestamps") {
            params.no_timestamps = true;
        } else if (arg == "-m" || arg == "--model") {
            params.model = argv[++i];
        } else if (arg == "-f" || arg == "--file") {
            params.fname_inp.push_back(argv[++i]);
        } else if (arg == "-h" || arg == "--help") {
            whisper_print_usage(argc, argv, params);
            exit(0);
        } else {
            fprintf(stderr, "error: unknown argument: %s\n", arg.c_str());
            whisper_print_usage(argc, argv, params);
            exit(0);
        }
    }

    return true;
}

void whisper_print_usage(int argc, char ** argv, const whisper_params & params) {
    fprintf(stderr, "\n");
    fprintf(stderr, "usage: %s [options] file0.wav file1.wav ...\n", argv[0]);
    fprintf(stderr, "\n");
    fprintf(stderr, "options:\n");
    fprintf(stderr, "  -h,       --help           show this help message and exit\n");
    fprintf(stderr, "  -s SEED,  --seed SEED      RNG seed (default: -1)\n");
    fprintf(stderr, "  -t N,     --threads N      number of threads to use during computation (default: %d)\n", params.n_threads);
    fprintf(stderr, "  -ot N,    --offset-t N     time offset in milliseconds (default: %d)\n", params.offset_t_ms);
    fprintf(stderr, "  -on N,    --offset-n N     segment index offset (default: %d)\n", params.offset_n);
    fprintf(stderr, "  -v,       --verbose        verbose output\n");
    fprintf(stderr, "            --translate      translate from source language to english\n");
    fprintf(stderr, "  -otxt,    --output-txt     output result in a text file\n");
    fprintf(stderr, "  -ovtt,    --output-vtt     output result in a vtt file\n");
    fprintf(stderr, "  -osrt,    --output-srt     output result in a srt file\n");
    fprintf(stderr, "  -ps,      --print_special  print special tokens\n");
    fprintf(stderr, "  -pc,      --print_colors   print colors\n");
    fprintf(stderr, "  -nt,      --no_timestamps  do not print timestamps\n");
    fprintf(stderr, "  -l LANG,  --language LANG  spoken language (default: %s)\n", params.language.c_str());
    fprintf(stderr, "  -m FNAME, --model FNAME    model path (default: %s)\n", params.model.c_str());
    fprintf(stderr, "  -f FNAME, --file FNAME     input WAV file path\n");
    fprintf(stderr, "\n");
}

void whisper_print_segment_callback(struct whisper_context * ctx, void * user_data) {
    const whisper_params & params = *(whisper_params *) user_data;

    const int n_segments = whisper_full_n_segments(ctx);

    // print the last segment
    const int i = n_segments - 1;
    if (i == 0) {
        printf("\n");
    }

    if (params.no_timestamps) {
        if (params.print_colors) {
            for (int j = 0; j < whisper_full_n_tokens(ctx, i); ++j) {
                if (params.print_special_tokens == false) {
                    const whisper_token id = whisper_full_get_token_id(ctx, i, j);
                    if (id >= whisper_token_eot(ctx)) {
                        continue;
                    }
                }

                const char * text = whisper_full_get_token_text(ctx, i, j);
                const float  p    = whisper_full_get_token_p   (ctx, i, j);

                const int col = std::max(0, std::min((int) k_colors.size(), (int) (std::pow(p, 3)*float(k_colors.size()))));

                printf("%s%s%s", k_colors[col].c_str(), text, "\033[0m");
            }
        } else {
            const char * text = whisper_full_get_segment_text(ctx, i);
            printf("%s", text);
        }
        fflush(stdout);
    } else {
        const int64_t t0 = whisper_full_get_segment_t0(ctx, i);
        const int64_t t1 = whisper_full_get_segment_t1(ctx, i);

        if (params.print_colors) {
            printf("[%s --> %s]  ", to_timestamp(t0).c_str(), to_timestamp(t1).c_str());
            for (int j = 0; j < whisper_full_n_tokens(ctx, i); ++j) {
                if (params.print_special_tokens == false) {
                    const whisper_token id = whisper_full_get_token_id(ctx, i, j);
                    if (id >= whisper_token_eot(ctx)) {
                        continue;
                    }
                }

                const char * text = whisper_full_get_token_text(ctx, i, j);
                const float  p    = whisper_full_get_token_p   (ctx, i, j);

                const int col = std::max(0, std::min((int) k_colors.size(), (int) (std::pow(p, 3)*float(k_colors.size()))));

                printf("%s%s%s", k_colors[col].c_str(), text, "\033[0m");
            }
            printf("\n");
        } else {
            const char * text = whisper_full_get_segment_text(ctx, i);

            printf("[%s --> %s]  %s\n", to_timestamp(t0).c_str(), to_timestamp(t1).c_str(), text);
        }
    }
}

bool output_txt(struct whisper_context * ctx, const char * fname) {
    std::ofstream fout(fname);
    if (!fout.is_open()) {
        fprintf(stderr, "%s: failed to open '%s' for writing\n", __func__, fname);
        return false;
    }

    fprintf(stderr, "%s: saving output to '%s'\n", __func__, fname);

    const int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) {
        const char * text = whisper_full_get_segment_text(ctx, i);
        fout << text;
    }

    return true;
}

bool output_vtt(struct whisper_context * ctx, const char * fname) {
    std::ofstream fout(fname);
    if (!fout.is_open()) {
        fprintf(stderr, "%s: failed to open '%s' for writing\n", __func__, fname);
        return 9;
    }

    fprintf(stderr, "%s: saving output to '%s'\n", __func__, fname);

    fout << "WEBVTT\n\n";

    const int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) {
        const char * text = whisper_full_get_segment_text(ctx, i);
        const int64_t t0 = whisper_full_get_segment_t0(ctx, i);
        const int64_t t1 = whisper_full_get_segment_t1(ctx, i);

        fout << to_timestamp(t0) << " --> " << to_timestamp(t1) << "\n";
        fout << text << "\n\n";
    }

    return true;
}

bool output_srt(struct whisper_context * ctx, const char * fname, const whisper_params & params) {
    std::ofstream fout(fname);
    if (!fout.is_open()) {
        fprintf(stderr, "%s: failed to open '%s' for writing\n", __func__, fname);
        return false;
    }

    fprintf(stderr, "%s: saving output to '%s'\n", __func__, fname);

    const int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) {
        const char * text = whisper_full_get_segment_text(ctx, i);
        const int64_t t0 = whisper_full_get_segment_t0(ctx, i);
        const int64_t t1 = whisper_full_get_segment_t1(ctx, i);

        fout << i + 1 + params.offset_n << "\n";
        fout << to_timestamp(t0) << " --> " << to_timestamp(t1) << "\n";
        fout << text << "\n\n";
    }

    return true;
}

int main(int argc, char ** argv) {
    whisper_params params;

    if (whisper_params_parse(argc, argv, params) == false) {
        return 1;
    }

    if (params.seed < 0) {
        params.seed = time(NULL);
    }

    if (params.fname_inp.empty()) {
        fprintf(stderr, "error: no input files specified\n");
        whisper_print_usage(argc, argv, params);
        return 2;
    }

    // whisper init

    struct whisper_context * ctx = whisper_init(params.model.c_str());

    for (int f = 0; f < (int) params.fname_inp.size(); ++f) {
        const auto fname_inp = params.fname_inp[f];

        // WAV input
        std::vector<float> pcmf32;
        {
            drwav wav;
            if (!drwav_init_file(&wav, fname_inp.c_str(), NULL)) {
                fprintf(stderr, "%s: failed to open WAV file '%s' - check your input\n", argv[0], fname_inp.c_str());
                whisper_print_usage(argc, argv, {});
                return 3;
            }

            if (wav.channels != 1 && wav.channels != 2) {
                fprintf(stderr, "%s: WAV file '%s' must be mono or stereo\n", argv[0], fname_inp.c_str());
                return 4;
            }

            if (wav.sampleRate != WHISPER_SAMPLE_RATE) {
                fprintf(stderr, "%s: WAV file '%s' must be 16 kHz\n", argv[0], fname_inp.c_str());
                return 5;
            }

            if (wav.bitsPerSample != 16) {
                fprintf(stderr, "%s: WAV file '%s' must be 16-bit\n", argv[0], fname_inp.c_str());
                return 6;
            }

            int n = wav.totalPCMFrameCount;

            std::vector<int16_t> pcm16;
            pcm16.resize(n*wav.channels);
            drwav_read_pcm_frames_s16(&wav, n, pcm16.data());
            drwav_uninit(&wav);

            // convert to mono, float
            pcmf32.resize(n);
            if (wav.channels == 1) {
                for (int i = 0; i < n; i++) {
                    pcmf32[i] = float(pcm16[i])/32768.0f;
                }
            } else {
                for (int i = 0; i < n; i++) {
                    pcmf32[i] = float(pcm16[2*i] + pcm16[2*i + 1])/65536.0f;
                }
            }
        }

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
            fprintf(stderr, "%s: processing '%s' (%d samples, %.1f sec), %d threads, lang = %s, task = %s, timestamps = %d ...\n",
                    __func__, fname_inp.c_str(), int(pcmf32.size()), float(pcmf32.size())/WHISPER_SAMPLE_RATE, params.n_threads,
                    params.language.c_str(),
                    params.translate ? "translate" : "transcribe",
                    params.no_timestamps ? 0 : 1);

            fprintf(stderr, "\n");
        }


        // run the inference
        {
            whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);

            wparams.print_realtime       = false;
            wparams.print_progress       = false;
            wparams.print_timestamps     = !params.no_timestamps;
            wparams.print_special_tokens = params.print_special_tokens;
            wparams.translate            = params.translate;
            wparams.language             = params.language.c_str();
            wparams.n_threads            = params.n_threads;
            wparams.offset_ms            = params.offset_t_ms;

            // this callback is called on each new segment
            if (!wparams.print_realtime) {
                wparams.new_segment_callback           = whisper_print_segment_callback;
                wparams.new_segment_callback_user_data = &params;
            }

            if (whisper_full(ctx, wparams, pcmf32.data(), pcmf32.size()) != 0) {
                fprintf(stderr, "%s: failed to process audio\n", argv[0]);
                return 7;
            }

            printf("\n");

            // output to text file
            if (params.output_txt) {
                const auto fname_txt = fname_inp + ".txt";
                output_txt(ctx, fname_txt.c_str());
            }

            // output to VTT file
            if (params.output_vtt) {
                const auto fname_vtt = fname_inp + ".vtt";
                output_vtt(ctx, fname_vtt.c_str());
            }

            // output to SRT file
            if (params.output_srt) {
                const auto fname_srt = fname_inp + ".srt";
                output_srt(ctx, fname_srt.c_str(), params);
            }
        }
    }

    whisper_print_timings(ctx);
    whisper_free(ctx);

    return 0;
}
