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
std::string to_timestamp(int64_t t, bool comma = false) {
    int64_t msec = t * 10;
    int64_t hr = msec / (1000 * 60 * 60);
    msec = msec - hr * (1000 * 60 * 60);
    int64_t min = msec / (1000 * 60);
    msec = msec - min * (1000 * 60);
    int64_t sec = msec / 1000;
    msec = msec - sec * 1000;

    char buf[32];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d%s%03d", (int) hr, (int) min, (int) sec, comma ? "," : ".", (int) msec);

    return std::string(buf);
}

int timestamp_to_sample(int64_t t, int n_samples) {
    return std::max(0, std::min((int) n_samples - 1, (int) ((t*WHISPER_SAMPLE_RATE)/100)));
}

// helper function to replace substrings
void replace_all(std::string & s, const std::string & search, const std::string & replace) {
    for (size_t pos = 0; ; pos += replace.length()) {
        pos = s.find(search, pos);
        if (pos == std::string::npos) break;
        s.erase(pos, search.length());
        s.insert(pos, replace);
    }
}

// command-line parameters
struct whisper_params {
    int32_t n_threads    = std::min(4, (int32_t) std::thread::hardware_concurrency());
    int32_t n_processors = 1;
    int32_t offset_t_ms  = 0;
    int32_t offset_n     = 0;
    int32_t duration_ms  = 0;
    int32_t max_context  = -1;
    int32_t max_len      = 0;

    float word_thold = 0.01f;

    bool speed_up       = false;
    bool translate      = false;
    bool diarize        = false;
    bool output_txt     = false;
    bool output_vtt     = false;
    bool output_srt     = false;
    bool output_wts     = false;
    bool print_special  = false;
    bool print_colors   = false;
    bool print_progress = false;
    bool no_timestamps  = false;

    std::string language = "en";
    std::string prompt;
    std::string model    = "models/ggml-base.en.bin";

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

        if (arg == "-h" || arg == "--help") {
            whisper_print_usage(argc, argv, params);
            exit(0);
        }
        else if (arg == "-t"    || arg == "--threads")        { params.n_threads      = std::stoi(argv[++i]); }
        else if (arg == "-p"    || arg == "--processors")     { params.n_processors   = std::stoi(argv[++i]); }
        else if (arg == "-ot"   || arg == "--offset-t")       { params.offset_t_ms    = std::stoi(argv[++i]); }
        else if (arg == "-on"   || arg == "--offset-n")       { params.offset_n       = std::stoi(argv[++i]); }
        else if (arg == "-d"    || arg == "--duration")       { params.duration_ms    = std::stoi(argv[++i]); }
        else if (arg == "-mc"   || arg == "--max-context")    { params.max_context    = std::stoi(argv[++i]); }
        else if (arg == "-ml"   || arg == "--max-len")        { params.max_len        = std::stoi(argv[++i]); }
        else if (arg == "-wt"   || arg == "--word-thold")     { params.word_thold     = std::stof(argv[++i]); }
        else if (arg == "-su"   || arg == "--speed-up")       { params.speed_up       = true; }
        else if (arg == "-tr"   || arg == "--translate")      { params.translate      = true; }
        else if (arg == "-di"   || arg == "--diarize")        { params.diarize        = true; }
        else if (arg == "-otxt" || arg == "--output-txt")     { params.output_txt     = true; }
        else if (arg == "-ovtt" || arg == "--output-vtt")     { params.output_vtt     = true; }
        else if (arg == "-osrt" || arg == "--output-srt")     { params.output_srt     = true; }
        else if (arg == "-owts" || arg == "--output-words")   { params.output_wts     = true; }
        else if (arg == "-ps"   || arg == "--print-special")  { params.print_special  = true; }
        else if (arg == "-pc"   || arg == "--print-colors")   { params.print_colors   = true; }
        else if (arg == "-pp"   || arg == "--print-progress") { params.print_progress = true; }
        else if (arg == "-nt"   || arg == "--no-timestamps")  { params.no_timestamps  = true; }
        else if (arg == "-l"    || arg == "--language")       { params.language       = argv[++i]; }
        else if (                  arg == "--prompt")         { params.prompt         = argv[++i]; }
        else if (arg == "-m"    || arg == "--model")          { params.model          = argv[++i]; }
        else if (arg == "-f"    || arg == "--file")           { params.fname_inp.emplace_back(argv[++i]); }
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
    fprintf(stderr, "usage: %s [options] file0.wav file1.wav ...\n", argv[0]);
    fprintf(stderr, "\n");
    fprintf(stderr, "options:\n");
    fprintf(stderr, "  -h,       --help           [default] show this help message and exit\n");
    fprintf(stderr, "  -t N,     --threads N      [%-7d] number of threads to use during computation\n",    params.n_threads);
    fprintf(stderr, "  -p N,     --processors N   [%-7d] number of processors to use during computation\n", params.n_processors);
    fprintf(stderr, "  -ot N,    --offset-t N     [%-7d] time offset in milliseconds\n",                    params.offset_t_ms);
    fprintf(stderr, "  -on N,    --offset-n N     [%-7d] segment index offset\n",                           params.offset_n);
    fprintf(stderr, "  -d  N,    --duration N     [%-7d] duration of audio to process in milliseconds\n",   params.duration_ms);
    fprintf(stderr, "  -mc N,    --max-context N  [%-7d] maximum number of text context tokens to store\n", params.max_context);
    fprintf(stderr, "  -ml N,    --max-len N      [%-7d] maximum segment length in characters\n",           params.max_len);
    fprintf(stderr, "  -wt N,    --word-thold N   [%-7.2f] word timestamp probability threshold\n",         params.word_thold);
    fprintf(stderr, "  -su,      --speed-up       [%-7s] speed up audio by x2 (reduced accuracy)\n",        params.speed_up ? "true" : "false");
    fprintf(stderr, "  -tr,      --translate      [%-7s] translate from source language to english\n",      params.translate ? "true" : "false");
    fprintf(stderr, "  -di,      --diarize        [%-7s] stereo audio diarization\n",                       params.diarize ? "true" : "false");
    fprintf(stderr, "  -otxt,    --output-txt     [%-7s] output result in a text file\n",                   params.output_txt ? "true" : "false");
    fprintf(stderr, "  -ovtt,    --output-vtt     [%-7s] output result in a vtt file\n",                    params.output_vtt ? "true" : "false");
    fprintf(stderr, "  -osrt,    --output-srt     [%-7s] output result in a srt file\n",                    params.output_srt ? "true" : "false");
    fprintf(stderr, "  -owts,    --output-words   [%-7s] output script for generating karaoke video\n",     params.output_wts ? "true" : "false");
    fprintf(stderr, "  -ps,      --print-special  [%-7s] print special tokens\n",                           params.print_special ? "true" : "false");
    fprintf(stderr, "  -pc,      --print-colors   [%-7s] print colors\n",                                   params.print_colors ? "true" : "false");
    fprintf(stderr, "  -pp,      --print-progress [%-7s] print progress\n",                                 params.print_progress ? "true" : "false");
    fprintf(stderr, "  -nt,      --no-timestamps  [%-7s] do not print timestamps\n",                        params.no_timestamps ? "false" : "true");
    fprintf(stderr, "  -l LANG,  --language LANG  [%-7s] spoken language ('auto' for auto-detect)\n",       params.language.c_str());
    fprintf(stderr, "            --prompt PROMPT  [%-7s] initial prompt\n",                                 params.prompt.c_str());
    fprintf(stderr, "  -m FNAME, --model FNAME    [%-7s] model path\n",                                     params.model.c_str());
    fprintf(stderr, "  -f FNAME, --file FNAME     [%-7s] input WAV file path\n",                            "");
    fprintf(stderr, "\n");
}

struct whisper_print_user_data {
    const whisper_params * params;

    const std::vector<std::vector<float>> * pcmf32s;
};

void whisper_print_segment_callback(struct whisper_context * ctx, int n_new, void * user_data) {
    const auto & params  = *((whisper_print_user_data *) user_data)->params;
    const auto & pcmf32s = *((whisper_print_user_data *) user_data)->pcmf32s;

    const int n_segments = whisper_full_n_segments(ctx);

    // print the last n_new segments
    const int s0 = n_segments - n_new;
    if (s0 == 0) {
        printf("\n");
    }

    for (int i = s0; i < n_segments; i++) {
        if (params.no_timestamps) {
            if (params.print_colors) {
                for (int j = 0; j < whisper_full_n_tokens(ctx, i); ++j) {
                    if (params.print_special == false) {
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

            std::string speaker;

            if (params.diarize && pcmf32s.size() == 2) {
                const int64_t n_samples = pcmf32s[0].size();

                const int64_t is0 = timestamp_to_sample(t0, n_samples);
                const int64_t is1 = timestamp_to_sample(t1, n_samples);

                double energy0 = 0.0f;
                double energy1 = 0.0f;

                for (int64_t j = is0; j < is1; j++) {
                    energy0 += fabs(pcmf32s[0][j]);
                    energy1 += fabs(pcmf32s[1][j]);
                }

                if (energy0 > 1.1*energy1) {
                    speaker = "(speaker 0)";
                } else if (energy1 > 1.1*energy0) {
                    speaker = "(speaker 1)";
                } else {
                    speaker = "(speaker ?)";
                }

                //printf("is0 = %lld, is1 = %lld, energy0 = %f, energy1 = %f, %s\n", is0, is1, energy0, energy1, speaker.c_str());
            }

            if (params.print_colors) {
                printf("[%s --> %s]  ", to_timestamp(t0).c_str(), to_timestamp(t1).c_str());
                for (int j = 0; j < whisper_full_n_tokens(ctx, i); ++j) {
                    if (params.print_special == false) {
                        const whisper_token id = whisper_full_get_token_id(ctx, i, j);
                        if (id >= whisper_token_eot(ctx)) {
                            continue;
                        }
                    }

                    const char * text = whisper_full_get_token_text(ctx, i, j);
                    const float  p    = whisper_full_get_token_p   (ctx, i, j);

                    const int col = std::max(0, std::min((int) k_colors.size(), (int) (std::pow(p, 3)*float(k_colors.size()))));

                    printf("%s%s%s%s", speaker.c_str(), k_colors[col].c_str(), text, "\033[0m");
                }
                printf("\n");
            } else {
                const char * text = whisper_full_get_segment_text(ctx, i);

                printf("[%s --> %s]  %s%s\n", to_timestamp(t0).c_str(), to_timestamp(t1).c_str(), speaker.c_str(), text);
            }
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
        fout << text << "\n";
    }

    return true;
}

bool output_vtt(struct whisper_context * ctx, const char * fname) {
    std::ofstream fout(fname);
    if (!fout.is_open()) {
        fprintf(stderr, "%s: failed to open '%s' for writing\n", __func__, fname);
        return false;
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
        fout << to_timestamp(t0, true) << " --> " << to_timestamp(t1, true) << "\n";
        fout << text << "\n\n";
    }

    return true;
}

// karaoke video generation
// outputs a bash script that uses ffmpeg to generate a video with the subtitles
// TODO: font parameter adjustments
bool output_wts(struct whisper_context * ctx, const char * fname, const char * fname_inp, const whisper_params & /*params*/, float t_sec) {
    std::ofstream fout(fname);

    fprintf(stderr, "%s: saving output to '%s'\n", __func__, fname);

    // TODO: become parameter
    static const char * font = "/System/Library/Fonts/Supplemental/Courier New Bold.ttf";

    fout << "#!/bin/bash" << "\n";
    fout << "\n";

    fout << "ffmpeg -i " << fname_inp << " -f lavfi -i color=size=1200x120:duration=" << t_sec << ":rate=25:color=black -vf \"";

    for (int i = 0; i < whisper_full_n_segments(ctx); i++) {
        const int64_t t0 = whisper_full_get_segment_t0(ctx, i);
        const int64_t t1 = whisper_full_get_segment_t1(ctx, i);

        const int n = whisper_full_n_tokens(ctx, i);

        std::vector<whisper_token_data> tokens(n);
        for (int j = 0; j < n; ++j) {
            tokens[j] = whisper_full_get_token_data(ctx, i, j);
        }

        if (i > 0) {
            fout << ",";
        }

        // background text
        fout << "drawtext=fontfile='" << font << "':fontsize=24:fontcolor=gray:x=(w-text_w)/2:y=h/2:text='':enable='between(t," << t0/100.0 << "," << t0/100.0 << ")'";

        bool is_first = true;

        for (int j = 0; j < n; ++j) {
            const auto & token = tokens[j];

            if (tokens[j].id >= whisper_token_eot(ctx)) {
                continue;
            }

            std::string txt_bg;
            std::string txt_fg; // highlight token
            std::string txt_ul; // underline

            txt_bg = "> ";
            txt_fg = "> ";
            txt_ul = "\\ \\ ";

            {
                for (int k = 0; k < n; ++k) {
                    const auto & token2 = tokens[k];

                    if (tokens[k].id >= whisper_token_eot(ctx)) {
                        continue;
                    }

                    const std::string txt = whisper_token_to_str(ctx, token2.id);

                    txt_bg += txt;

                    if (k == j) {
                        for (int l = 0; l < (int) txt.size(); ++l) {
                            txt_fg += txt[l];
                            txt_ul += "_";
                        }
                        txt_fg += "|";
                    } else {
                        for (int l = 0; l < (int) txt.size(); ++l) {
                            txt_fg += "\\ ";
                            txt_ul += "\\ ";
                        }
                    }
                }

                ::replace_all(txt_bg, "'", "\u2019");
                ::replace_all(txt_bg, "\"", "\\\"");
                ::replace_all(txt_fg, "'", "\u2019");
                ::replace_all(txt_fg, "\"", "\\\"");
            }

            if (is_first) {
                // background text
                fout << ",drawtext=fontfile='" << font << "':fontsize=24:fontcolor=gray:x=(w-text_w)/2:y=h/2:text='" << txt_bg << "':enable='between(t," << t0/100.0 << "," << t1/100.0 << ")'";
                is_first = false;
            }

            // foreground text
            fout << ",drawtext=fontfile='" << font << "':fontsize=24:fontcolor=lightgreen:x=(w-text_w)/2+8:y=h/2:text='" << txt_fg << "':enable='between(t," << token.t0/100.0 << "," << token.t1/100.0 << ")'";

            // underline
            fout << ",drawtext=fontfile='" << font << "':fontsize=24:fontcolor=lightgreen:x=(w-text_w)/2+8:y=h/2+16:text='" << txt_ul << "':enable='between(t," << token.t0/100.0 << "," << token.t1/100.0 << ")'";
        }
    }

    fout << "\" -c:v libx264 -pix_fmt yuv420p -y " << fname_inp << ".mp4" << "\n";

    fout << "\n\n";
    fout << "echo \"Your video has been saved to " << fname_inp << ".mp4\"" << "\n";
    fout << "\n";
    fout << "echo \"  ffplay " << fname_inp << ".mp4\"\n";
    fout << "\n";

    fout.close();

    fprintf(stderr, "%s: run 'source %s' to generate karaoke video\n", __func__, fname);

    return true;
}

int main(int argc, char ** argv) {
    whisper_params params;

    if (whisper_params_parse(argc, argv, params) == false) {
        return 1;
    }

    if (params.fname_inp.empty()) {
        fprintf(stderr, "error: no input files specified\n");
        whisper_print_usage(argc, argv, params);
        return 2;
    }

    if (params.language != "auto" && whisper_lang_id(params.language.c_str()) == -1) {
        fprintf(stderr, "error: unknown language '%s'\n", params.language.c_str());
        whisper_print_usage(argc, argv, params);
        exit(0);
    }

    // whisper init

    struct whisper_context * ctx = whisper_init(params.model.c_str());

    if (ctx == nullptr) {
        fprintf(stderr, "error: failed to initialize whisper context\n");
        return 3;
    }

    // initial prompt
    std::vector<whisper_token> prompt_tokens;

    if (!params.prompt.empty()) {
        prompt_tokens.resize(1024);
        prompt_tokens.resize(whisper_tokenize(ctx, params.prompt.c_str(), prompt_tokens.data(), prompt_tokens.size()));

        fprintf(stderr, "\n");
        fprintf(stderr, "initial prompt: '%s'\n", params.prompt.c_str());
        fprintf(stderr, "initial tokens: [ ");
        for (int i = 0; i < (int) prompt_tokens.size(); ++i) {
            fprintf(stderr, "%d ", prompt_tokens[i]);
        }
        fprintf(stderr, "]\n");
    }

    for (int f = 0; f < (int) params.fname_inp.size(); ++f) {
        const auto fname_inp = params.fname_inp[f];

        std::vector<float> pcmf32; // mono-channel F32 PCM
        std::vector<std::vector<float>> pcmf32s; // stereo-channel F32 PCM

        // WAV input
        {
            drwav wav;
            std::vector<uint8_t> wav_data; // used for pipe input from stdin

            if (fname_inp == "-") {
                {
                    uint8_t buf[1024];
                    while (true)
                    {
                        const size_t n = fread(buf, 1, sizeof(buf), stdin);
                        if (n == 0) {
                            break;
                        }
                        wav_data.insert(wav_data.end(), buf, buf + n);
                    }
                }

                if (drwav_init_memory(&wav, wav_data.data(), wav_data.size(), nullptr) == false) {
                    fprintf(stderr, "error: failed to open WAV file from stdin\n");
                    return 4;
                }

                fprintf(stderr, "%s: read %zu bytes from stdin\n", __func__, wav_data.size());
            }
            else if (drwav_init_file(&wav, fname_inp.c_str(), nullptr) == false) {
                fprintf(stderr, "error: failed to open '%s' as WAV file\n", fname_inp.c_str());
                return 5;
            }

            if (wav.channels != 1 && wav.channels != 2) {
                fprintf(stderr, "%s: WAV file '%s' must be mono or stereo\n", argv[0], fname_inp.c_str());
                return 6;
            }

            if (params.diarize && wav.channels != 2 && params.no_timestamps == false) {
                fprintf(stderr, "%s: WAV file '%s' must be stereo for diarization and timestamps have to be enabled\n", argv[0], fname_inp.c_str());
                return 6;
            }

            if (wav.sampleRate != WHISPER_SAMPLE_RATE) {
                fprintf(stderr, "%s: WAV file '%s' must be 16 kHz\n", argv[0], fname_inp.c_str());
                return 8;
            }

            if (wav.bitsPerSample != 16) {
                fprintf(stderr, "%s: WAV file '%s' must be 16-bit\n", argv[0], fname_inp.c_str());
                return 9;
            }

            const uint64_t n = wav_data.empty() ? wav.totalPCMFrameCount : wav_data.size()/(wav.channels*wav.bitsPerSample/8);

            std::vector<int16_t> pcm16;
            pcm16.resize(n*wav.channels);
            drwav_read_pcm_frames_s16(&wav, n, pcm16.data());
            drwav_uninit(&wav);

            // convert to mono, float
            pcmf32.resize(n);
            if (wav.channels == 1) {
                for (uint64_t i = 0; i < n; i++) {
                    pcmf32[i] = float(pcm16[i])/32768.0f;
                }
            } else {
                for (uint64_t i = 0; i < n; i++) {
                    pcmf32[i] = float(pcm16[2*i] + pcm16[2*i + 1])/65536.0f;
                }
            }

            if (params.diarize) {
                // convert to stereo, float
                pcmf32s.resize(2);

                pcmf32s[0].resize(n);
                pcmf32s[1].resize(n);
                for (uint64_t i = 0; i < n; i++) {
                    pcmf32s[0][i] = float(pcm16[2*i])/32768.0f;
                    pcmf32s[1][i] = float(pcm16[2*i + 1])/32768.0f;
                }
            }
        }

        // print system information
        {
            fprintf(stderr, "\n");
            fprintf(stderr, "system_info: n_threads = %d / %d | %s\n",
                    params.n_threads*params.n_processors, std::thread::hardware_concurrency(), whisper_print_system_info());
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
            fprintf(stderr, "%s: processing '%s' (%d samples, %.1f sec), %d threads, %d processors, lang = %s, task = %s, timestamps = %d ...\n",
                    __func__, fname_inp.c_str(), int(pcmf32.size()), float(pcmf32.size())/WHISPER_SAMPLE_RATE,
                    params.n_threads, params.n_processors,
                    params.language.c_str(),
                    params.translate ? "translate" : "transcribe",
                    params.no_timestamps ? 0 : 1);

            fprintf(stderr, "\n");
        }

        // run the inference
        {
            whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);

            wparams.print_realtime   = false;
            wparams.print_progress   = params.print_progress;
            wparams.print_timestamps = !params.no_timestamps;
            wparams.print_special    = params.print_special;
            wparams.translate        = params.translate;
            wparams.language         = params.language.c_str();
            wparams.n_threads        = params.n_threads;
            wparams.n_max_text_ctx   = params.max_context >= 0 ? params.max_context : wparams.n_max_text_ctx;
            wparams.offset_ms        = params.offset_t_ms;
            wparams.duration_ms      = params.duration_ms;

            wparams.token_timestamps = params.output_wts || params.max_len > 0;
            wparams.thold_pt         = params.word_thold;
            wparams.max_len          = params.output_wts && params.max_len == 0 ? 60 : params.max_len;

            wparams.speed_up         = params.speed_up;

            wparams.prompt_tokens    = prompt_tokens.empty() ? nullptr : prompt_tokens.data();
            wparams.prompt_n_tokens  = prompt_tokens.empty() ? 0       : prompt_tokens.size();

            whisper_print_user_data user_data = { &params, &pcmf32s };

            // this callback is called on each new segment
            if (!wparams.print_realtime) {
                wparams.new_segment_callback           = whisper_print_segment_callback;
                wparams.new_segment_callback_user_data = &user_data;
            }

            // example for abort mechanism
            // in this example, we do not abort the processing, but we could if the flag is set to true
            // the callback is called before every encoder run - if it returns false, the processing is aborted
            {
                static bool is_aborted = false; // NOTE: this should be atomic to avoid data race

                wparams.encoder_begin_callback = [](struct whisper_context * /*ctx*/, void * user_data) {
                    bool is_aborted = *(bool*)user_data;
                    return !is_aborted;
                };
                wparams.encoder_begin_callback_user_data = &is_aborted;
            }

            if (whisper_full_parallel(ctx, wparams, pcmf32.data(), pcmf32.size(), params.n_processors) != 0) {
                fprintf(stderr, "%s: failed to process audio\n", argv[0]);
                return 10;
            }
        }

        // output stuff
        {
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

            // output to WTS file
            if (params.output_wts) {
                const auto fname_wts = fname_inp + ".wts";
                output_wts(ctx, fname_wts.c_str(), fname_inp.c_str(), params, float(pcmf32.size() + 1000)/WHISPER_SAMPLE_RATE);
            }
        }
    }

    whisper_print_timings(ctx);
    whisper_free(ctx);

    return 0;
}
