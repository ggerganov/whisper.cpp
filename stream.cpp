// Real-time speech recognition of input from a microphone
//
// A very quick-n-dirty implementation serving mainly as a proof of concept.

#include "whisper.h"

// third-party utilities
// use your favorite implementations
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

#include <SDL.h>
#include <SDL_audio.h>

#include <cassert>
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
    bool no_timestamps        = true;

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

//
// SDL Audio capture
//

SDL_AudioDeviceID g_dev_id_in = 0;

bool audio_sdl_init(const int capture_id) {
    if (g_dev_id_in) {
        fprintf(stderr, "%s: already initialized\n", __func__);
        return false;
    }

    if (g_dev_id_in == 0) {
        SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);

        if (SDL_Init(SDL_INIT_AUDIO) < 0) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s\n", SDL_GetError());
            return (1);
        }

        SDL_SetHintWithPriority(SDL_HINT_AUDIO_RESAMPLING_MODE, "medium", SDL_HINT_OVERRIDE);

        {
            int nDevices = SDL_GetNumAudioDevices(SDL_TRUE);
            printf("%s: found %d capture devices:\n", __func__, nDevices);
            for (int i = 0; i < nDevices; i++) {
                printf("%s:    - Capture device #%d: '%s'\n", __func__, i, SDL_GetAudioDeviceName(i, SDL_TRUE));
            }
        }
    }

    if (g_dev_id_in == 0) {
        SDL_AudioSpec capture_spec_requested;
        SDL_AudioSpec capture_spec_obtained;

        SDL_zero(capture_spec_requested);
        SDL_zero(capture_spec_obtained);

        capture_spec_requested.freq     = WHISPER_SAMPLE_RATE;
        capture_spec_requested.format   = AUDIO_F32;
        capture_spec_requested.channels = 1;
        capture_spec_requested.samples  = 1024;

        if (capture_id >= 0) {
            printf("%s: attempt to open capture device %d : '%s' ...\n", __func__, capture_id, SDL_GetAudioDeviceName(capture_id, SDL_TRUE));
            g_dev_id_in = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(capture_id, SDL_TRUE), SDL_TRUE, &capture_spec_requested, &capture_spec_obtained, 0);
        } else {
            printf("%s: attempt to open default capture device ...\n", __func__);
            g_dev_id_in = SDL_OpenAudioDevice(nullptr, SDL_TRUE, &capture_spec_requested, &capture_spec_obtained, 0);
        }
        if (!g_dev_id_in) {
            printf("%s: couldn't open an audio device for capture: %s!\n", __func__, SDL_GetError());
            g_dev_id_in = 0;
        } else {
            printf("%s: obtained spec for input device (SDL Id = %d):\n", __func__, g_dev_id_in);
            printf("%s:     - sample rate:       %d\n", __func__, capture_spec_obtained.freq);
            printf("%s:     - format:            %d (required: %d)\n", __func__, capture_spec_obtained.format, capture_spec_requested.format);
            printf("%s:     - channels:          %d (required: %d)\n", __func__, capture_spec_obtained.channels, capture_spec_requested.channels);
            printf("%s:     - samples per frame: %d\n", __func__, capture_spec_obtained.samples);
        }
    }


    return true;
}

///////////////////////////

int main(int argc, char ** argv) {
    whisper_params params;

    if (whisper_params_parse(argc, argv, params) == false) {
        return 1;
    }

    if (params.seed < 0) {
        params.seed = time(NULL);
    }

    // init audio

    if (!audio_sdl_init(-1)) {
        fprintf(stderr, "%s: audio_sdl_init() failed!\n", __func__);
        return 1;
    }

    // whisper init

    struct whisper_context * ctx = whisper_init(params.model.c_str());

    const int n_samples_30s = 30*WHISPER_SAMPLE_RATE;
    std::vector<float> pcmf32(n_samples_30s, 0.0f);
    std::vector<float> pcmf32_old;

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

    SDL_PauseAudioDevice(g_dev_id_in, 0);

    bool is_running = true;

    // main audio loop
    while (is_running) {
        // process SDL events:
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    is_running = false;
                    break;
                default:
                    break;
            }
        }

        // process 3 seconds of new audio
        while (SDL_GetQueuedAudioSize(g_dev_id_in) < 3*WHISPER_SAMPLE_RATE*sizeof(float)) {
            SDL_Delay(1);
        }
        const int n_samples_new = SDL_GetQueuedAudioSize(g_dev_id_in)/sizeof(float);

        // take one second from previous iteration
        // TODO: better strategy
        const int n_samples_take = std::min((int) pcmf32_old.size(), std::max(0, n_samples_30s/30 - n_samples_new));

        //printf("processing: take = %d, new = %d, old = %d\n", n_samples_take, n_samples_new, (int) pcmf32_old.size());

        pcmf32.resize(n_samples_new + n_samples_take);

        for (int i = 0; i < n_samples_take; i++) {
            pcmf32[i] = pcmf32_old[pcmf32_old.size() - n_samples_take + i];
        }

        SDL_DequeueAudio(g_dev_id_in, pcmf32.data() + n_samples_take, n_samples_new*sizeof(float));

        pcmf32_old = pcmf32;

        // run the inference
        {
            whisper_full_params wparams = whisper_full_default_params(WHISPER_DECODE_GREEDY);

            wparams.print_progress       = false;
            wparams.print_special_tokens = params.print_special_tokens;
            wparams.print_realtime       = false;
            wparams.print_timestamps     = !params.no_timestamps;
            wparams.translate            = params.translate;
            wparams.language             = params.language.c_str();
            wparams.n_threads            = params.n_threads;

            if (whisper_full(ctx, wparams, pcmf32.data(), pcmf32.size()) != 0) {
                fprintf(stderr, "%s: failed to process audio\n", argv[0]);
                return 6;
            }

            // print result;
            {
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
    }

    whisper_print_timings(ctx);
    whisper_free(ctx);

    return 0;
}
