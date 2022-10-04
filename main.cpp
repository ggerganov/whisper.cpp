#include "whisper.h"

// third-party utilities
// use your favorite implementations
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"

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
    const int64_t t_main_start_us = get_time_us();

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

        if (wav.sampleRate != SAMPLE_RATE) {
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
            for (size_t i = 0; i < n; i++) {
                pcmf32[i] = float(pcm16[i])/32768.0f;
            }
        } else {
            for (size_t i = 0; i < n; i++) {
                pcmf32[i] = float(pcm16[2*i] + pcm16[2*i + 1])/65536.0f;
            }
        }
    }

    // compute log mel spectrogram
    if (whisper_pcm_to_mel(ctx, pcmf32.data(), pcmf32.size(), params.n_threads) != 0) {
        fprintf(stderr, "%s: failed to compute log mel spectrogram\n", argv[0]);
        return 6;
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
                __func__, int(pcmf32.size()), float(pcmf32.size())/SAMPLE_RATE, params.n_threads,
                params.language.c_str(),
                params.translate ? "translate" : "transcribe",
                params.no_timestamps ? 0 : 1);
        printf("\n");
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

    whisper_print_timings(ctx);
    whisper_free(ctx);

    return 0;
}
