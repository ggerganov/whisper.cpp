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

int64_t get_time_us() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

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

struct whisper_result {
    whisper_token id;
    int64_t t;
};

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

        capture_spec_requested.freq     = SAMPLE_RATE;
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
    const int64_t t_main_start_us = get_time_us();

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

    const int n_samples_30s = 30*SAMPLE_RATE;
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
                __func__, int(pcmf32.size()), float(pcmf32.size())/SAMPLE_RATE, params.n_threads,
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
        while ((int) SDL_GetQueuedAudioSize(g_dev_id_in) < 3*SAMPLE_RATE*sizeof(float)) {
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

        // compute log mel spectrogram
        if (whisper_pcm_to_mel(ctx, pcmf32.data(), pcmf32.size(), params.n_threads) != 0) {
            fprintf(stderr, "%s: failed to compute log mel spectrogram\n", argv[0]);
            return 6;
        }

        // the accumulated text context so far
        std::vector<whisper_token> prompt_past = { };

        // these tokens determine the task that will be performed
        std::vector<whisper_token> prompt_init = { whisper_token_sot(ctx) };
        if (whisper_is_multilingual(ctx)) {
            prompt_init.push_back(whisper_token_sot(ctx) + 1 + whisper_lang_id(params.language.c_str()));
            if (params.translate) {
                prompt_init.push_back(whisper_token_translate());
            } else {
                prompt_init.push_back(whisper_token_transcribe());
            }
        }

        // the generated text including timestamps
        //std::vector<whisper_result> result_all;

        // main loop
        int seek = 0;
        while (true) {
            if (seek >= whisper_n_len(ctx)) {
                break;
            }

            // encode audio features starting at offset seek
            if (whisper_encode(ctx, seek, params.n_threads) != 0) {
                fprintf(stderr, "%s: failed to encode\n", __func__);
                return 7;
            }

            std::vector<whisper_token> prompt;

            int n_past = 0;

            // if we have already generated some text, use it as a prompt to condition the next generation
            if (prompt_past.size() > 0) {
                int n_take = std::min(whisper_n_text_ctx(ctx)/2, int(prompt_past.size()));

                prompt = { whisper_token_prev(ctx) };
                prompt.insert(prompt.begin() + 1, prompt_past.end() - n_take, prompt_past.end());

                prompt_past.clear();
                prompt_past.insert(prompt_past.end(), prompt.begin() + 1, prompt.end());
            }

            prompt.insert(prompt.end(), prompt_init.begin(), prompt_init.end());

            bool done = false;
            int seek_delta = 100*CHUNK_SIZE;
            whisper_token last_id = 0;

            // print the prompt
            //printf("\n\n");
            //for (int i = 0; i < prompt.size(); i++) {
            //    printf("%s: prompt[%d] = %s\n", __func__, i, vocab.id_to_token[prompt[i]].c_str());
            //}
            //printf("\n\n");

            // the accumulated transcription in the current interation
            int result_len = 0;
            std::vector<whisper_result> result_cur;

            for (int i = 0; i < whisper_n_text_ctx(ctx)/2 - 4; ++i) {
                if (whisper_decode(ctx, prompt.data(), prompt.size(), n_past, params.n_threads) != 0) {
                    fprintf(stderr, "%s: failed to decode\n", __func__);
                    return 8;
                }

                n_past += prompt.size();
                prompt.clear();

                // very basic greedy sampling strategy:
                //
                //   - always take the most probable token
                //
                // more sophisticated sampling strategies could be implemented here, but we keep it simple
                // feel free to experiment!
                //
                {
                    const int n_vocab = whisper_n_vocab(ctx);

                    whisper_token id  = 0;
                    whisper_token tid = whisper_token_beg(ctx);

                    id = whisper_sample_best(ctx, result_len == 0);
                    if (i > 0) {
                        tid = whisper_sample_timestamp(ctx);
                    }

                    // update sliding window
                    if (id > whisper_token_beg(ctx)) {
                        seek_delta = 2*(id - whisper_token_beg(ctx));
                        result_len = i + 1;
                    }
                    last_id = id;

                    // add it to the context
                    prompt.push_back(id);
                    result_cur.push_back({ id, seek + 2*(tid - whisper_token_beg(ctx)) });

                    //printf("%s: %s\n", __func__, vocab.id_to_token[id].c_str());

                    // end of text token
                    if (id == whisper_token_eot(ctx)) {
                        break;
                    }
                }

                if (done) {
                    break;
                }
            }

            result_cur.resize(result_len);
            //result_all.insert(result_all.end(), result_cur.begin(), result_cur.end());

            for (const auto & r : result_cur) {
                prompt_past.push_back(r.id);
            }

            // print the text from this iteration
            if (result_cur.size() > 0) {
                auto t0 = result_cur.front().t;

                std::string text = "";
                for (int i = 0; i < result_cur.size(); i++) {
                    if (params.print_special_tokens == false && result_cur[i].id >= whisper_token_eot(ctx)) {
                    } else {
                        text += whisper_token_to_str(ctx, result_cur[i].id);
                    }
                    if (result_cur[i].id > whisper_token_beg(ctx)) {
                        const auto t1 = result_cur[i].t;
                        if (!text.empty()) {
                            if (params.no_timestamps) {
                                printf ("%s", text.c_str());
                                fflush(stdout);
                            } else {
                                printf ("[%s --> %s]  %s\n", to_timestamp(t0).c_str(), to_timestamp(t1).c_str(), text.c_str());
                            }
                        }
                        text = "";
                        while (result_cur[i].id > whisper_token_beg(ctx) && i < result_cur.size()) {
                            i++;
                        }
                        i--;
                        t0 = result_cur[i].t;
                    }
                }

                if (!text.empty()) {
                    printf ("[%s --> %s]  %s\n", to_timestamp(t0).c_str(), to_timestamp(seek + seek_delta).c_str(), text.c_str());
                }
            }

            seek += seek_delta;
        }
    }

    whisper_print_timings(ctx);
    whisper_free(ctx);

    return 0;
}
