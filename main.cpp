#include "whisper.h"

// third-party utilities
// use your favorite implementations
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#include <cstdio>
#include <string>
#include <thread>
#include <vector>

//  500 -> 00:05.000
// 6000 -> 01:00.000
std::string to_timestamp(int64_t t) {
    int64_t sec = t/100;
    int64_t msec = t - sec*100;
    int64_t min = sec/60;
    sec = sec - min*60;

    char buf[32];
    snprintf(buf, sizeof(buf), "%02d:%02d.%03d", (int) min, (int) sec, (int) msec);

    return std::string(buf);
}

// command-line parameters
struct whisper_params {
    int32_t seed      = -1; // RNG seed, not used currently
    int32_t n_threads = std::min(4, (int32_t) std::thread::hardware_concurrency());

    bool verbose              = false;
    bool translate            = false;
    bool print_special_tokens = false;
    bool no_timestamps        = false;

    std::string language  = "en";
    std::string model     = "models/ggml-base.en.bin";
    std::string fname_inp = "samples/jfk.wav";
};

void whisper_print_usage(int argc, char ** argv, const whisper_params & params);

bool whisper_params_parse(int argc, char ** argv, whisper_params & params) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-s" || arg == "--seed") {
            params.seed = std::stoi(argv[++i]);
        } else if (arg == "-t" || arg == "--threads") {
            params.n_threads = std::stoi(argv[++i]);
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
        } else if (arg == "-ps" || arg == "--print_special") {
            params.print_special_tokens = true;
        } else if (arg == "-nt" || arg == "--no_timestamps") {
            params.no_timestamps = true;
        } else if (arg == "-m" || arg == "--model") {
            params.model = argv[++i];
        } else if (arg == "-f" || arg == "--file") {
            params.fname_inp = argv[++i];
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
    fprintf(stderr, "usage: %s [options]\n", argv[0]);
    fprintf(stderr, "\n");
    fprintf(stderr, "options:\n");
    fprintf(stderr, "  -h,       --help           show this help message and exit\n");
    fprintf(stderr, "  -s SEED,  --seed SEED      RNG seed (default: -1)\n");
    fprintf(stderr, "  -t N,     --threads N      number of threads to use during computation (default: %d)\n", params.n_threads);
    fprintf(stderr, "  -v,       --verbose        verbose output\n");
    fprintf(stderr, "            --translate      translate from source language to english\n");
    fprintf(stderr, "  -ps,      --print_special  print special tokens\n");
    fprintf(stderr, "  -nt,      --no_timestamps  do not print timestamps\n");
    fprintf(stderr, "  -l LANG,  --language LANG  spoken language (default: %s)\n", params.language.c_str());
    fprintf(stderr, "  -m FNAME, --model FNAME    model path (default: %s)\n", params.model.c_str());
    fprintf(stderr, "  -f FNAME, --file FNAME     input WAV file path (default: %s)\n", params.fname_inp.c_str());
    fprintf(stderr, "\n");
}

int main(int argc, char ** argv) {
    whisper_params params;

    if (whisper_params_parse(argc, argv, params) == false) {
        return 1;
    }

    if (params.seed < 0) {
        params.seed = time(NULL);
    }

    // whisper init

    struct whisper_context * ctx = whisper_init(params.model.c_str());

    // WAV input
    std::vector<float> pcmf32;
    {
        drwav wav;
        if (!drwav_init_file(&wav, params.fname_inp.c_str(), NULL)) {
            fprintf(stderr, "%s: failed to open WAV file '%s' - check your input\n", argv[0], params.fname_inp.c_str());
            whisper_print_usage(argc, argv, {});
            return 2;
        }

        if (wav.channels != 1 && wav.channels != 2) {
            fprintf(stderr, "%s: WAV file '%s' must be mono or stereo\n", argv[0], params.fname_inp.c_str());
            return 3;
        }

        if (wav.sampleRate != WHISPER_SAMPLE_RATE) {
            fprintf(stderr, "%s: WAV file '%s' must be 16 kHz\n", argv[0], params.fname_inp.c_str());
            return 4;
        }

        if (wav.bitsPerSample != 16) {
            fprintf(stderr, "%s: WAV file '%s' must be 16-bit\n", argv[0], params.fname_inp.c_str());
            return 5;
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
        printf("\n");
        if (!whisper_is_multilingual(ctx)) {
            if (params.language != "en" || params.translate) {
                params.language = "en";
                params.translate = false;
                printf("%s: WARNING: model is not multilingual, ignoring language and translation options\n", __func__);
            }
        }
        printf("%s: processing %d samples (%.1f sec), %d threads, lang = %s, task = %s, timestamps = %d ...\n",
                __func__, int(pcmf32.size()), float(pcmf32.size())/WHISPER_SAMPLE_RATE, params.n_threads,
                params.language.c_str(),
                params.translate ? "translate" : "transcribe",
                params.no_timestamps ? 0 : 1);
        printf("\n");
    }

    // run the inference
    {
        whisper_full_params wparams = whisper_full_default_params(WHISPER_DECODE_GREEDY);

        wparams.print_realtime       = true;
        wparams.print_progress       = false;
        wparams.print_timestamps     = !params.no_timestamps;
        wparams.print_special_tokens = params.print_special_tokens;
        wparams.translate            = params.translate;
        wparams.language             = params.language.c_str();
        wparams.n_threads            = params.n_threads;

        if (whisper_full(ctx, wparams, pcmf32.data(), pcmf32.size()) != 0) {
            fprintf(stderr, "%s: failed to process audio\n", argv[0]);
            return 6;
        }

        // print result;
        if (!wparams.print_realtime) {
            printf("\n");

            const int n_segments = whisper_full_n_segments(ctx);
            for (int i = 0; i < n_segments; ++i) {
                const char * text = whisper_full_get_segment_text(ctx, i);

                if (params.no_timestamps) {
                    printf ("%s", text);
                    fflush(stdout);
                } else {
                    const int64_t t0 = whisper_full_get_segment_t0(ctx, i);
                    const int64_t t1 = whisper_full_get_segment_t1(ctx, i);

                    printf ("[%s --> %s]  %s\n", to_timestamp(t0).c_str(), to_timestamp(t1).c_str(), text);
                }
            }
        }
    }

    whisper_print_timings(ctx);
    whisper_free(ctx);

    return 0;
}
