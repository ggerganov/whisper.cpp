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

void replace_all(std::string & s, const std::string & search, const std::string & replace) {
    for (size_t pos = 0; ; pos += replace.length()) {
        pos = s.find(search, pos);
        if (pos == std::string::npos) break;
        s.erase(pos, search.length());
        s.insert(pos, replace);
    }
}

// a cost-function that is high for text that takes longer to pronounce
float voice_length(const std::string & text) {
    float res = 0.0f;

    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == ' ') {
            res += 0.01f;
        } else if (text[i] == ',') {
            res += 2.00f;
        } else if (text[i] == '.') {
            res += 3.00f;
        } else if (text[i] == '!') {
            res += 3.00f;
        } else if (text[i] == '?') {
            res += 3.00f;
        } else if (text[i] >= '0' && text[i] <= '9') {
            res += 3.00f;
        } else {
            res += 1.00f;
        }
    }

    return res;
}

// command-line parameters
struct whisper_params {
    int32_t seed         = -1; // RNG seed, not used currently
    int32_t n_threads    = std::min(4, (int32_t) std::thread::hardware_concurrency());
    int32_t n_processors = 1;
    int32_t offset_t_ms  = 0;
    int32_t offset_n     = 0;
    int32_t max_context  = -1;

    float word_thold = 0.01f;

    bool verbose              = false;
    bool translate            = false;
    bool output_txt           = false;
    bool output_vtt           = false;
    bool output_srt           = false;
    bool output_wts           = false;
    bool output_ann           = true;
    bool print_special_tokens = false;
    bool print_colors         = false;
    bool no_timestamps        = false;

    std::string language  = "en";
    std::string model     = "models/ggml-medium.en.bin";

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
        } else if (arg == "-p" || arg == "--processors") {
            params.n_processors = std::stoi(argv[++i]);
        } else if (arg == "-ot" || arg == "--offset-t") {
            params.offset_t_ms = std::stoi(argv[++i]);
        } else if (arg == "-on" || arg == "--offset-n") {
            params.offset_n = std::stoi(argv[++i]);
        } else if (arg == "-mc" || arg == "--max-context") {
            params.max_context = std::stoi(argv[++i]);
        } else if (arg == "-wt" || arg == "--word-thold") {
            params.word_thold = std::stof(argv[++i]);
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
        } else if (arg == "-oann" || arg == "--output-annotations") {
            params.output_ann = true;
        } else if (arg == "-owts" || arg == "--output-words") {
            params.output_wts = true;
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
    fprintf(stderr, "usage (16kHz): %s [options] file0.wav file1.wav ...\n", argv[0]);
    fprintf(stderr, "\n");
    fprintf(stderr, "options:\n");
    fprintf(stderr, "  -h,       --help                 show this help message and exit\n");
    fprintf(stderr, "  -s SEED,  --seed SEED            RNG seed (default: -1)\n");
    fprintf(stderr, "  -t N,     --threads N            number of threads to use during computation (default: %d)\n", params.n_threads);
    fprintf(stderr, "  -p N,     --processors N         number of processors to use during computation (default: %d)\n", params.n_processors);
    fprintf(stderr, "  -ot N,    --offset-t N           time offset in milliseconds (default: %d)\n", params.offset_t_ms);
    fprintf(stderr, "  -on N,    --offset-n N           segment index offset (default: %d)\n", params.offset_n);
    fprintf(stderr, "  -mc N,    --max-context N        maximum number of text context tokens to store (default: max)\n");
    fprintf(stderr, "  -wt N,    --word-thold N         word timestamp probability threshold (default: %f)\n", params.word_thold);
    fprintf(stderr, "  -v,       --verbose              verbose output\n");
    fprintf(stderr, "            --translate            translate from source language to english\n");
    fprintf(stderr, "  -otxt,    --output-txt           output result in a text file\n");
    fprintf(stderr, "  -ovtt,    --output-vtt           output result in a vtt file\n");
    fprintf(stderr, "  -osrt,    --output-srt           output result in a srt file\n");
    fprintf(stderr, "  -owts,    --output-words         output word-level timestamps to a text file\n");
    fprintf(stderr, "  -oann,    --output-annotations   output word-level timestamps to a text file\n");
    fprintf(stderr, "  -ps,      --print_special        print special tokens\n");
    fprintf(stderr, "  -pc,      --print_colors         print colors\n");
    fprintf(stderr, "  -nt,      --no_timestamps        do not print timestamps\n");
    fprintf(stderr, "  -l LANG,  --language LANG        spoken language (default: %s)\n", params.language.c_str());
    fprintf(stderr, "  -m FNAME, --model FNAME          model path (default: %s)\n", params.model.c_str());
    fprintf(stderr, "  -f FNAME, --file FNAME           input WAV file path\n");
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
        fout << to_timestamp(t0, true) << " --> " << to_timestamp(t1, true) << "\n";
        fout << text << "\n\n";
    }

    return true;
}

// word-level timestamps (experimental)
// TODO: make ffmpeg output optional
// TODO: extra pass to detect unused speech and assign to tokens
// TODO: font parameter adjustments
// TODO: move to whisper.h/whisper.cpp and add parameter to select max line-length of subtitles
bool output_wts(struct whisper_context * ctx, const char * fname, const char * fname_inp, const whisper_params & params, const std::vector<float> & pcmf32) {
    std::vector<float> pcm_avg(pcmf32.size(), 0);

    // average the fabs of the signal
    {
        const int hw = 32;

        for (int i = 0; i < pcmf32.size(); i++) {
            float sum = 0;
            for (int j = -hw; j <= hw; j++) {
                if (i + j >= 0 && i + j < pcmf32.size()) {
                    sum += fabs(pcmf32[i + j]);
                }
            }
            pcm_avg[i] = sum/(2*hw + 1);
        }
    }

    struct token_info {
        int64_t t0 = -1;
        int64_t t1 = -1;

        int64_t tt0 = -1;
        int64_t tt1 = -1;

        whisper_token id;
        whisper_token tid;

        float p     = 0.0f;
        float pt    = 0.0f;
        float ptsum = 0.0f;

        std::string text;
        float vlen = 0.0f; // voice length of this token
    };

    int64_t t_beg  = 0;
    int64_t t_last = 0;

    whisper_token tid_last = 0;

    std::ofstream fout(fname);

    fprintf(stderr, "%s: saving output to '%s'\n", __func__, fname);

    fout << "!/bin/bash" << "\n";
    fout << "\n";

    fout << "ffmpeg -i " << fname_inp << " -f lavfi -i color=size=1200x120:duration=" << float(pcmf32.size() + 1000)/WHISPER_SAMPLE_RATE << ":rate=25:color=black -vf \"";

    bool is_first = true;

    for (int i = 0; i < whisper_full_n_segments(ctx); i++) {
        const int64_t t0 = whisper_full_get_segment_t0(ctx, i);
        const int64_t t1 = whisper_full_get_segment_t1(ctx, i);

        const char *text = whisper_full_get_segment_text(ctx, i);

        const int s0 = std::max(0,                   (int) (t0*WHISPER_SAMPLE_RATE/100));
        const int s1 = std::min((int) pcmf32.size(), (int) (t1*WHISPER_SAMPLE_RATE/100));

        const int n = whisper_full_n_tokens(ctx, i);

        std::vector<token_info> tokens(n);

        if (n <= 1) {
            continue;
        }

        for (int j = 0; j < n; ++j) {
            struct whisper_token_data token = whisper_full_get_token_data(ctx, i, j);

            if (j == 0) {
                if (token.id == whisper_token_beg(ctx)) {
                    tokens[j    ].t0 = t0;
                    tokens[j    ].t1 = t0;
                    tokens[j + 1].t0 = t0;

                    t_beg  = t0;
                    t_last = t0;
                    tid_last = whisper_token_beg(ctx);
                } else {
                    tokens[j    ].t0 = t_last;
                }
            }

            const int64_t tt = t_beg + 2*(token.tid - whisper_token_beg(ctx));

            tokens[j].id    = token.id;
            tokens[j].tid   = token.tid;
            tokens[j].p     = token.p;
            tokens[j].pt    = token.pt;
            tokens[j].ptsum = token.ptsum;

            tokens[j].text = whisper_token_to_str(ctx, token.id);
            tokens[j].vlen = voice_length(tokens[j].text);

            if (token.pt > params.word_thold && token.ptsum > 0.01 && token.tid > tid_last && tt <= t1) {
                if (j > 0) {
                    tokens[j - 1].t1 = tt;
                }
                tokens[j].t0 = tt;
                tid_last = token.tid;
            }
        }

        tokens[n - 2].t1 = t1;
        tokens[n - 1].t0 = t1;
        tokens[n - 1].t1 = t1;

        t_last = t1;

        // find intervals of tokens with unknown timestamps
        // fill the timestamps by proportionally splitting the interval based on the token voice lengths
        {
            int p0 = 0;
            int p1 = 0;
            while (true) {
                while (p1 < n && tokens[p1].t1 < 0) {
                    p1++;
                }

                if (p1 >= n) {
                    p1--;
                }

                if (p1 > p0) {
                    double psum = 0.0;
                    for (int j = p0; j <= p1; j++) {
                        psum += tokens[j].vlen;
                    }

                    //printf("analyzing %d - %d, psum = %f\n", p0, p1, psum);

                    const double dt = tokens[p1].t1 - tokens[p0].t0;

                    // split the time proportionally to the voice length
                    for (int j = p0 + 1; j <= p1; j++) {
                        const double ct = tokens[j - 1].t0 + dt*tokens[j - 1].vlen/psum;

                        tokens[j - 1].t1 = ct;
                        tokens[j    ].t0 = ct;
                    }
                }

                p1++;
                p0 = p1;
                if (p1 >= n) {
                    break;
                }
            }
        }

        // fix up (just in case)
        for (int j = 0; j < n - 1; j++) {
            if (tokens[j].t1 < 0) {
                tokens[j + 1].t0 = tokens[j].t1;
            }

            if (j > 0) {
                if (tokens[j - 1].t1 > tokens[j].t0) {
                    tokens[j].t0 = tokens[j - 1].t1;
                    tokens[j].t1 = std::max(tokens[j].t0, tokens[j].t1);
                }
            }

            tokens[j].tt0 = tokens[j].t0;
            tokens[j].tt1 = tokens[j].t1;
        }

        // VAD
        // expand or contract tokens based on voice activity
        {
            const int hw = WHISPER_SAMPLE_RATE/8;

            for (int j = 0; j < n; j++) {
                if (tokens[j].id >= whisper_token_eot(ctx)) {
                    continue;
                }

                const int64_t t0 = tokens[j].t0;
                const int64_t t1 = tokens[j].t1;

                int s0 = std::max(0,                        (int) (t0*WHISPER_SAMPLE_RATE/100));
                int s1 = std::min((int) pcmf32.size() - 1,  (int) (t1*WHISPER_SAMPLE_RATE/100));

                const int ss0 = std::max(0,                       (int) (t0*WHISPER_SAMPLE_RATE/100) - hw);
                const int ss1 = std::min((int) pcmf32.size() - 1, (int) (t1*WHISPER_SAMPLE_RATE/100) + hw);

                const int n = ss1 - ss0;

                float sum = 0.0f;

                for (int k = ss0; k < ss1; k++) {
                    sum += pcm_avg[k];
                }

                const float thold = 0.5*sum/n;

                {
                    int k = s0;
                    if (pcm_avg[k] > thold && j > 0) {
                        while (k > 0 && pcm_avg[k] > thold) {
                            k--;
                        }
                        tokens[j].t0 = (int64_t) (100*k/WHISPER_SAMPLE_RATE);
                        if (tokens[j].t0 < tokens[j - 1].t1) {
                            tokens[j].t0 = tokens[j - 1].t1;
                        } else {
                            s0 = k;
                        }
                    } else {
                        while (pcm_avg[k] < thold && k < s1) {
                            k++;
                        }
                        s0 = k;
                        tokens[j].t0 = 100*k/WHISPER_SAMPLE_RATE;
                    }
                }

                {
                    int k = s1;
                    if (pcm_avg[k] > thold) {
                        while (k < (int) pcmf32.size() - 1 && pcm_avg[k] > thold) {
                            k++;
                        }
                        tokens[j].t1 = 100*k/WHISPER_SAMPLE_RATE;
                        if (j < n - 1 && tokens[j].t1 > tokens[j + 1].t0) {
                            tokens[j].t1 = tokens[j + 1].t0;
                        } else {
                            s1 = k;
                        }
                    } else {
                        while (pcm_avg[k] < thold && k > s0) {
                            k--;
                        }
                        s1 = k;
                        tokens[j].t1 = 100*k/WHISPER_SAMPLE_RATE;
                    }
                }
            }
        }

        // fixed token expand (optional)
        {
            const int t_expand = 0;

            for (int j = 0; j < n; j++) {
                if (j > 0) {
                    tokens[j].t0 = std::max(0, (int) (tokens[j].t0 - t_expand));
                }
                if (j < n - 1) {
                    tokens[j].t1 = tokens[j].t1 + t_expand;
                }
            }
        }

        // debug info
        // TODO: toggle via parameter
        for (int j = 0; j < n; ++j) {
            const auto & token = tokens[j];
            const auto tt = token.pt > params.word_thold && token.ptsum > 0.01 ? whisper_token_to_str(ctx, token.tid) : "[?]";
            printf("%s: %10s %6.3f %6.3f %6.3f %6.3f %5d %5d '%s'\n", __func__,
                    tt, token.p, token.pt, token.ptsum, token.vlen, (int) token.t0, (int) token.t1, token.text.c_str());

            if (tokens[j].id >= whisper_token_eot(ctx)) {
                continue;
            }

            //printf("[%s --> %s] %s\n", to_timestamp(token.t0).c_str(), to_timestamp(token.t1).c_str(), whisper_token_to_str(ctx, token.id));

            //fout << "# " << to_timestamp(token.t0) << " --> " << to_timestamp(token.t1) << " " << whisper_token_to_str(ctx, token.id) << "\n";
        }

        // TODO: become parameters
        static const int line_wrap = 60;
        static const char * font = "/System/Library/Fonts/Supplemental/Courier New Bold.ttf";

        if (!is_first) {
            fout << ",";
        }

        // background text
        fout << "drawtext=fontfile='" << font << "':fontsize=24:fontcolor=gray:x=(w-text_w)/2:y=h/2:text='':enable='between(t," << t0/100.0 << "," << t0/100.0 << ")'";

        is_first = false;

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
                int ncnt = 0;
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

                    ncnt += txt.size();

                    if (ncnt > line_wrap) {
                        if (k < j) {
                            txt_bg = "> ";
                            txt_fg = "> ";
                            txt_ul = "\\ \\ ";
                            ncnt = 0;
                        } else {
                            break;
                        }
                    }
                }

                ::replace_all(txt_bg, "'", "’");
                ::replace_all(txt_bg, "\"", "\\\"");
                ::replace_all(txt_fg, "'", "’");
                ::replace_all(txt_fg, "\"", "\\\"");
            }

            // background text
            fout << ",drawtext=fontfile='" << font << "':fontsize=24:fontcolor=gray:x=(w-text_w)/2:y=h/2:text='" << txt_bg << "':enable='between(t," << token.tt0/100.0 << "," << token.tt1/100.0 << ")'";

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


bool output_ann(struct whisper_context * ctx, const char * fname, const char * fname_inp, const whisper_params & params, const std::vector<float> & pcmf32) {
    std::vector<float> pcm_avg(pcmf32.size(), 0);

    // average the fabs of the signal
    {
        const int hw = 32;

        for (int i = 0; i < pcmf32.size(); i++) {
            float sum = 0;
            for (int j = -hw; j <= hw; j++) {
                if (i + j >= 0 && i + j < pcmf32.size()) {
                    sum += fabs(pcmf32[i + j]);
                }
            }
            pcm_avg[i] = sum/(2*hw + 1);
        }
    }

    struct token_info {
        int64_t t0 = -1;
        int64_t t1 = -1;

        int64_t tt0 = -1;
        int64_t tt1 = -1;

        whisper_token id;
        whisper_token tid;

        float p     = 0.0f;
        float pt    = 0.0f;
        float ptsum = 0.0f;

        std::string text;
        float vlen = 0.0f; // voice length of this token
    };

    int64_t t_beg  = 0;
    int64_t t_last = 0;

    whisper_token tid_last = 0;

    FILE * fp;
    fp = fopen (fname, "w+");

    fprintf(stderr, "%s: saving output to '%s'\n", __func__, fname);


    bool is_first = true;

    for (int i = 0; i < whisper_full_n_segments(ctx); i++) {
        const int64_t t0 = whisper_full_get_segment_t0(ctx, i);
        const int64_t t1 = whisper_full_get_segment_t1(ctx, i);

        const char *text = whisper_full_get_segment_text(ctx, i);

        const int s0 = std::max(0,                   (int) (t0*WHISPER_SAMPLE_RATE/100));
        const int s1 = std::min((int) pcmf32.size(), (int) (t1*WHISPER_SAMPLE_RATE/100));

        const int n = whisper_full_n_tokens(ctx, i);

        std::vector<token_info> tokens(n);

        if (n <= 1) {
            continue;
        }

        for (int j = 0; j < n; ++j) {
            struct whisper_token_data token = whisper_full_get_token_data(ctx, i, j);

            if (j == 0) {
                if (token.id == whisper_token_beg(ctx)) {
                    tokens[j    ].t0 = t0;
                    tokens[j    ].t1 = t0;
                    tokens[j + 1].t0 = t0;

                    t_beg  = t0;
                    t_last = t0;
                    tid_last = whisper_token_beg(ctx);
                } else {
                    tokens[j    ].t0 = t_last;
                }
            }

            const int64_t tt = t_beg + 2*(token.tid - whisper_token_beg(ctx));

            tokens[j].id    = token.id;
            tokens[j].tid   = token.tid;
            tokens[j].p     = token.p;
            tokens[j].pt    = token.pt;
            tokens[j].ptsum = token.ptsum;

            tokens[j].text = whisper_token_to_str(ctx, token.id);
            tokens[j].vlen = voice_length(tokens[j].text);

            if (token.pt > params.word_thold && token.ptsum > 0.01 && token.tid > tid_last && tt <= t1) {
                if (j > 0) {
                    tokens[j - 1].t1 = tt;
                }
                tokens[j].t0 = tt;
                tid_last = token.tid;
            }
        }

        tokens[n - 2].t1 = t1;
        tokens[n - 1].t0 = t1;
        tokens[n - 1].t1 = t1;

        t_last = t1;

        // find intervals of tokens with unknown timestamps
        // fill the timestamps by proportionally splitting the interval based on the token voice lengths
        {
            int p0 = 0;
            int p1 = 0;
            while (true) {
                while (p1 < n && tokens[p1].t1 < 0) {
                    p1++;
                }

                if (p1 >= n) {
                    p1--;
                }

                if (p1 > p0) {
                    double psum = 0.0;
                    for (int j = p0; j <= p1; j++) {
                        psum += tokens[j].vlen;
                    }

                    //printf("analyzing %d - %d, psum = %f\n", p0, p1, psum);

                    const double dt = tokens[p1].t1 - tokens[p0].t0;

                    // split the time proportionally to the voice length
                    for (int j = p0 + 1; j <= p1; j++) {
                        const double ct = tokens[j - 1].t0 + dt*tokens[j - 1].vlen/psum;

                        tokens[j - 1].t1 = ct;
                        tokens[j    ].t0 = ct;
                    }
                }

                p1++;
                p0 = p1;
                if (p1 >= n) {
                    break;
                }
            }
        }

        // fix up (just in case)
        for (int j = 0; j < n - 1; j++) {
            if (tokens[j].t1 < 0) {
                tokens[j + 1].t0 = tokens[j].t1;
            }

            if (j > 0) {
                if (tokens[j - 1].t1 > tokens[j].t0) {
                    tokens[j].t0 = tokens[j - 1].t1;
                    tokens[j].t1 = std::max(tokens[j].t0, tokens[j].t1);
                }
            }

            tokens[j].tt0 = tokens[j].t0;
            tokens[j].tt1 = tokens[j].t1;
        }

        // VAD
        // expand or contract tokens based on voice activity
        {
            const int hw = WHISPER_SAMPLE_RATE/8;

            for (int j = 0; j < n; j++) {
                if (tokens[j].id >= whisper_token_eot(ctx)) {
                    continue;
                }

                const int64_t t0 = tokens[j].t0;
                const int64_t t1 = tokens[j].t1;

                int s0 = std::max(0,                        (int) (t0*WHISPER_SAMPLE_RATE/100));
                int s1 = std::min((int) pcmf32.size() - 1,  (int) (t1*WHISPER_SAMPLE_RATE/100));

                const int ss0 = std::max(0,                       (int) (t0*WHISPER_SAMPLE_RATE/100) - hw);
                const int ss1 = std::min((int) pcmf32.size() - 1, (int) (t1*WHISPER_SAMPLE_RATE/100) + hw);

                const int n = ss1 - ss0;

                float sum = 0.0f;

                for (int k = ss0; k < ss1; k++) {
                    sum += pcm_avg[k];
                }

                const float thold = 0.5*sum/n;

                {
                    int k = s0;
                    if (pcm_avg[k] > thold && j > 0) {
                        while (k > 0 && pcm_avg[k] > thold) {
                            k--;
                        }
                        tokens[j].t0 = (int64_t) (100*k/WHISPER_SAMPLE_RATE);
                        if (tokens[j].t0 < tokens[j - 1].t1) {
                            tokens[j].t0 = tokens[j - 1].t1;
                        } else {
                            s0 = k;
                        }
                    } else {
                        while (pcm_avg[k] < thold && k < s1) {
                            k++;
                        }
                        s0 = k;
                        tokens[j].t0 = 100*k/WHISPER_SAMPLE_RATE;
                    }
                }

                {
                    int k = s1;
                    if (pcm_avg[k] > thold) {
                        while (k < (int) pcmf32.size() - 1 && pcm_avg[k] > thold) {
                            k++;
                        }
                        tokens[j].t1 = 100*k/WHISPER_SAMPLE_RATE;
                        if (j < n - 1 && tokens[j].t1 > tokens[j + 1].t0) {
                            tokens[j].t1 = tokens[j + 1].t0;
                        } else {
                            s1 = k;
                        }
                    } else {
                        while (pcm_avg[k] < thold && k > s0) {
                            k--;
                        }
                        s1 = k;
                        tokens[j].t1 = 100*k/WHISPER_SAMPLE_RATE;
                    }
                }
            }
        }

        // fixed token expand (optional)
        {
            const int t_expand = 0;

            for (int j = 0; j < n; j++) {
                if (j > 0) {
                    tokens[j].t0 = std::max(0, (int) (tokens[j].t0 - t_expand));
                }
                if (j < n - 1) {
                    tokens[j].t1 = tokens[j].t1 + t_expand;
                }
            }
        }

        // debug info
        // TODO: toggle via parameter
        fprintf(fp, "tt token.p token.pt token.ptsum token.vlen token.t0 token.t1 token.text\n");
        for (int j = 0; j < n; ++j) {
            const auto & token = tokens[j];
            const auto tt = token.pt > params.word_thold && token.ptsum > 0.01 ? whisper_token_to_str(ctx, token.tid) : "[?]";
            fprintf(fp, "%10s %6.3f %6.3f %6.3f %6.3f %5d %5d '%s'\n",
                    tt, token.p, token.pt, token.ptsum, token.vlen, (int) token.t0, (int) token.t1, token.text.c_str());

            if (tokens[j].id >= whisper_token_eot(ctx)) {
                continue;
            }

            //printf("[%s --> %s] %s\n", to_timestamp(token.t0).c_str(), to_timestamp(token.t1).c_str(), whisper_token_to_str(ctx, token.id));

            //fout << "# " << to_timestamp(token.t0) << " --> " << to_timestamp(token.t1) << " " << whisper_token_to_str(ctx, token.id) << "\n";
        }
    }

    

    fclose(fp);
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

    if (ctx == nullptr) {
        fprintf(stderr, "error: failed to initialize whisper context\n");
        return 3;
    }

    for (int f = 0; f < (int) params.fname_inp.size(); ++f) {
        const auto fname_inp = params.fname_inp[f];

        // WAV input
        std::vector<float> pcmf32;
        {
            drwav wav;
            if (!drwav_init_file(&wav, fname_inp.c_str(), NULL)) {
                fprintf(stderr, "%s: failed to open WAV file '%s' - check your input\n", argv[0], fname_inp.c_str());
                whisper_print_usage(argc, argv, {});
                return 4;
            }

            if (wav.channels != 1 && wav.channels != 2) {
                fprintf(stderr, "%s: WAV file '%s' must be mono or stereo\n", argv[0], fname_inp.c_str());
                return 5;
            }

            if (wav.sampleRate != WHISPER_SAMPLE_RATE) {
                fprintf(stderr, "%s: WAV file '%s' must be 16 kHz\n", argv[0], fname_inp.c_str());
                return 6;
            }

            if (wav.bitsPerSample != 16) {
                fprintf(stderr, "%s: WAV file '%s' must be 16-bit\n", argv[0], fname_inp.c_str());
                return 7;
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

            wparams.print_realtime       = false;
            wparams.print_progress       = false;
            wparams.print_timestamps     = !params.no_timestamps;
            wparams.print_special_tokens = params.print_special_tokens;
            wparams.translate            = params.translate;
            wparams.language             = params.language.c_str();
            wparams.n_threads            = params.n_threads;
            wparams.n_max_text_ctx       = params.max_context >= 0 ? params.max_context : wparams.n_max_text_ctx;
            wparams.offset_ms            = params.offset_t_ms;

            // this callback is called on each new segment
            if (!wparams.print_realtime) {
                wparams.new_segment_callback           = whisper_print_segment_callback;
                wparams.new_segment_callback_user_data = &params;
            }

            if (whisper_full_parallel(ctx, wparams, pcmf32.data(), pcmf32.size(), params.n_processors) != 0) {
                fprintf(stderr, "%s: failed to process audio\n", argv[0]);
                return 8;
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
                output_wts(ctx, fname_wts.c_str(), fname_inp.c_str(), params, pcmf32);
            }
            
            // output to text file
            if (params.output_ann) {
                const auto fname_wts = fname_inp + ".text";
                output_ann(ctx, fname_wts.c_str(), fname_inp.c_str(), params, pcmf32);
            }
        }
    }

    whisper_print_timings(ctx);
    whisper_free(ctx);

    return 0;
}
