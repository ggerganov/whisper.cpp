#include "common.h"
#include "whisper.h"
#include "console.h"

#include <cmath>
#include <fstream>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>
#include <cstring>

#if defined(_MSC_VER)
#pragma warning(disable: 4244 4267) // possible loss of data
#endif

// Terminal color map. 10 colors grouped in ranges [0.0, 0.1, ..., 0.9]
// Lowest is red, middle is yellow, highest is green.
const std::vector<std::string> k_colors = {
#if WIN32
    "\x1b[38;5;196m", "\x1b[38;5;202m", "\x1b[38;5;208m", "\x1b[38;5;214m", "\x1b[38;5;220m",
    "\x1b[38;5;226m", "\x1b[38;5;190m", "\x1b[38;5;154m", "\x1b[38;5;118m", "\x1b[38;5;82m"
#else
    "\033[38;5;196m", "\033[38;5;202m", "\033[38;5;208m", "\033[38;5;214m", "\033[38;5;220m",
    "\033[38;5;226m", "\033[38;5;190m", "\033[38;5;154m", "\033[38;5;118m", "\033[38;5;82m"
#endif
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

    return {std::string(buf)};
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
    int32_t n_processors =  1;
    int32_t offset_t_ms  =  0;
    int32_t offset_n     =  0;
    int32_t duration_ms  =  0;
    int32_t progress_step =  5;
    int32_t max_context  = -1;
    int32_t max_len      =  0;
    int32_t best_of      =  2;
    int32_t beam_size    = -1;

    float word_thold    =  0.01f;
    float entropy_thold =  2.40f;
    float logprob_thold = -1.00f;

    bool speed_up        = false;
    bool debug_mode      = false;
    bool translate       = false;
    bool detect_language = false;
    bool diarize         = false;
    bool tinydiarize     = false;
    bool split_on_word   = false;
    bool no_fallback     = false;
    bool output_txt      = false;
    bool output_vtt      = false;
    bool output_srt      = false;
    bool output_wts      = false;
    bool output_csv      = false;
    bool output_jsn      = false;
    bool output_lrc      = false;
    bool print_special   = false;
    bool print_colors    = false;
    bool print_progress  = false;
    bool no_timestamps   = false;
    bool log_score       = false;

    std::string language  = "en";
    std::string prompt;
    std::string font_path = "/System/Library/Fonts/Supplemental/Courier New Bold.ttf";
    std::string model     = "models/ggml-base.en.bin";

    // [TDRZ] speaker turn string
    std::string tdrz_speaker_turn = " [SPEAKER_TURN]"; // TODO: set from command line

    std::string openvino_encode_device = "CPU";

    std::vector<std::string> fname_inp = {};
    std::vector<std::string> fname_out = {};
};

void whisper_print_usage(std::vector<ustring> & args, const whisper_params & params);

bool whisper_params_parse(std::vector<ustring> & args, whisper_params & params) {
    for (int i = 1; i < args.size(); i++) {
        ustring & arg = args[i];

        if (equal(arg, "-")){
            params.fname_inp.push_back(ConvertUTF16toUTF8(arg));
            continue;
        }

        if (!equal(arg[0], '-')) {
            params.fname_inp.push_back(ConvertUTF16toUTF8(arg));
            continue;
        }

        if (equal(arg, "-h") || equal(arg, "--help")) {
            whisper_print_usage(args, params);
            exit(0);
        }
        else if (equal(arg, "-t")    || equal(arg, "--threads"))         { params.n_threads       = std::stoi(args[++i]); }
        else if (equal(arg, "-p")    || equal(arg, "--processors"))      { params.n_processors    = std::stoi(args[++i]); }
        else if (equal(arg, "-ot")   || equal(arg, "--offset-t"))        { params.offset_t_ms     = std::stoi(args[++i]); }
        else if (equal(arg, "-on")   || equal(arg, "--offset-n"))        { params.offset_n        = std::stoi(args[++i]); }
        else if (equal(arg, "-d")    || equal(arg, "--duration"))        { params.duration_ms     = std::stoi(args[++i]); }
        else if (equal(arg, "-mc")   || equal(arg, "--max-context"))     { params.max_context     = std::stoi(args[++i]); }
        else if (equal(arg, "-ml")   || equal(arg, "--max-len"))         { params.max_len         = std::stoi(args[++i]); }
        else if (equal(arg, "-bo")   || equal(arg, "--best-of"))         { params.best_of         = std::stoi(args[++i]); }
        else if (equal(arg, "-bs")   || equal(arg, "--beam-size"))       { params.beam_size       = std::stoi(args[++i]); }
        else if (equal(arg, "-wt")   || equal(arg, "--word-thold"))      { params.word_thold      = std::stof(args[++i]); }
        else if (equal(arg, "-et")   || equal(arg, "--entropy-thold"))   { params.entropy_thold   = std::stof(args[++i]); }
        else if (equal(arg, "-lpt")  || equal(arg, "--logprob-thold"))   { params.logprob_thold   = std::stof(args[++i]); }
        // else if (arg == "-su"   || arg == "--speed-up")        { params.speed_up        = true; }
        else if (equal(arg, "-debug")|| equal(arg, "--debug-mode"))      { params.debug_mode      = true; }
        else if (equal(arg, "-tr")   || equal(arg, "--translate"))       { params.translate       = true; }
        else if (equal(arg, "-di")   || equal(arg, "--diarize"))         { params.diarize         = true; }
        else if (equal(arg, "-tdrz") || equal(arg, "--tinydiarize"))     { params.tinydiarize     = true; }
        else if (equal(arg, "-sow")  || equal(arg, "--split-on-word"))   { params.split_on_word   = true; }
        else if (equal(arg, "-nf")   || equal(arg, "--no-fallback"))     { params.no_fallback     = true; }
        else if (equal(arg, "-otxt") || equal(arg, "--output-txt"))      { params.output_txt      = true; }
        else if (equal(arg, "-ovtt") || equal(arg, "--output-vtt"))      { params.output_vtt      = true; }
        else if (equal(arg, "-osrt") || equal(arg, "--output-srt"))      { params.output_srt      = true; }
        else if (equal(arg , "-owts")|| equal(arg, "--output-words"))    { params.output_wts      = true; }
        else if (equal(arg, "-olrc") || equal(arg, "--output-lrc"))      { params.output_lrc      = true; }
        else if (equal(arg, "-fp")   || equal(arg, "--font-path"))       { params.font_path       = ConvertUTF16toUTF8(args[++i]); }
        else if (equal(arg, "-ocsv") || equal(arg, "--output-csv"))      { params.output_csv      = true; }
        else if (equal(arg, "-oj")   || equal(arg, "--output-json"))     { params.output_jsn      = true; }
        else if (equal(arg, "-of")   || equal(arg, "--output-file"))     { params.fname_out.emplace_back(ConvertUTF16toUTF8(args[++i])); }
        else if (equal(arg, "-ps")   || equal(arg, "--print-special"))   { params.print_special   = true; }
        else if (equal(arg, "-pc")   || equal(arg, "--print-colors"))    { params.print_colors    = true; }
        else if (equal(arg, "-pp")   || equal(arg, "--print-progress"))  { params.print_progress  = true; }
        else if (equal(arg, "-nt")   || equal(arg, "--no-timestamps"))   { params.no_timestamps   = true; }
        else if (equal(arg, "-l")    || equal(arg, "--language"))        { params.language        = ConvertUTF16toUTF8(args[++i]); }
        else if (equal(arg, "-dl")   || equal(arg, "--detect-language")) { params.detect_language = true; }
        else if (                             equal(arg, "--prompt"))          { params.prompt          = ConvertUTF16toUTF8(args[++i]); }
        else if (equal(arg, "-m")    || equal(arg, "--model"))           { params.model           = ConvertUTF16toUTF8(args[++i]); }
        else if (equal(arg, "-f")    || equal(arg, "--file"))            { params.fname_inp.emplace_back(ConvertUTF16toUTF8(args[++i])); }
        else if (equal(arg, "-oved") || equal(arg, "--ov-e-device"))     { params.openvino_encode_device = ConvertUTF16toUTF8(args[++i]); }
        else if (equal(arg, "-ls")   || equal(arg, "--log-score"))       { params.log_score = true; }
        else {
            fuprintf(stderr, "error: unknown argument: %s\n", arg.c_str());
            whisper_print_usage(args, params);
            exit(0);
        }
    }

    return true;
}

void whisper_print_usage(std::vector<ustring> & args, const whisper_params & params) {
    fuprintf(stderr, "\n");
    fuprintf(stderr, "usage: %s [options] file0.wav file1.wav ...\n", args[0].c_str());
    fuprintf(stderr, "\n");
    fuprintf(stderr, "options:\n");
    fuprintf(stderr, "  -h,        --help              [default] show this help message and exit\n");
    fuprintf(stderr, "  -t N,      --threads N         [%-7d] number of threads to use during computation\n",    params.n_threads);
    fuprintf(stderr, "  -p N,      --processors N      [%-7d] number of processors to use during computation\n", params.n_processors);
    fuprintf(stderr, "  -ot N,     --offset-t N        [%-7d] time offset in milliseconds\n",                    params.offset_t_ms);
    fuprintf(stderr, "  -on N,     --offset-n N        [%-7d] segment index offset\n",                           params.offset_n);
    fuprintf(stderr, "  -d  N,     --duration N        [%-7d] duration of audio to process in milliseconds\n",   params.duration_ms);
    fuprintf(stderr, "  -mc N,     --max-context N     [%-7d] maximum number of text context tokens to store\n", params.max_context);
    fuprintf(stderr, "  -ml N,     --max-len N         [%-7d] maximum segment length in characters\n",           params.max_len);
    fuprintf(stderr, "  -sow,      --split-on-word     [%-7s] split on word rather than on token\n",             bool2ustr(params.split_on_word).c_str());
    fuprintf(stderr, "  -bo N,     --best-of N         [%-7d] number of best candidates to keep\n",              params.best_of);
    fuprintf(stderr, "  -bs N,     --beam-size N       [%-7d] beam size for beam search\n",                      params.beam_size);
    fuprintf(stderr, "  -wt N,     --word-thold N      [%-7.2f] word timestamp probability threshold\n",         params.word_thold);
    fuprintf(stderr, "  -et N,     --entropy-thold N   [%-7.2f] entropy threshold for decoder fail\n",           params.entropy_thold);
    fuprintf(stderr, "  -lpt N,    --logprob-thold N   [%-7.2f] log probability threshold for decoder fail\n",   params.logprob_thold);
    // fuprintf(stderr, "  -su,       --speed-up          [%-7s] speed up audio by x2 (reduced accuracy)\n",        params.speed_up ? "true" : "false");
    fuprintf(stderr, "  -debug,    --debug-mode        [%-7s] enable debug mode (eg. dump log_mel)\n",           bool2ustr(params.debug_mode).c_str());
    fuprintf(stderr, "  -tr,       --translate         [%-7s] translate from source language to english\n",      bool2ustr(params.translate).c_str());
    fuprintf(stderr, "  -di,       --diarize           [%-7s] stereo audio diarization\n",                       bool2ustr(params.diarize).c_str());
    fuprintf(stderr, "  -tdrz,     --tinydiarize       [%-7s] enable tinydiarize (requires a tdrz model)\n",     bool2ustr(params.tinydiarize).c_str());
    fuprintf(stderr, "  -nf,       --no-fallback       [%-7s] do not use temperature fallback while decoding\n", bool2ustr(params.no_fallback).c_str());
    fuprintf(stderr, "  -otxt,     --output-txt        [%-7s] output result in a text file\n",                   bool2ustr(params.output_txt).c_str());
    fuprintf(stderr, "  -ovtt,     --output-vtt        [%-7s] output result in a vtt file\n",                    bool2ustr(params.output_vtt).c_str());
    fuprintf(stderr, "  -osrt,     --output-srt        [%-7s] output result in a srt file\n",                    bool2ustr(params.output_srt).c_str());
    fuprintf(stderr, "  -olrc,     --output-lrc        [%-7s] output result in a lrc file\n",                    bool2ustr(params.output_lrc).c_str());
    fuprintf(stderr, "  -owts,     --output-words      [%-7s] output script for generating karaoke video\n",     bool2ustr(params.output_wts).c_str());
    fuprintf(stderr, "  -fp,       --font-path         [%-7s] path to a monospace font for karaoke video\n",     ConvertUTF8toUTF16(params.font_path).c_str());
    fuprintf(stderr, "  -ocsv,     --output-csv        [%-7s] output result in a CSV file\n",                    bool2ustr(params.output_csv).c_str());
    fuprintf(stderr, "  -oj,       --output-json       [%-7s] output result in a JSON file\n",                   bool2ustr(params.output_jsn).c_str());
    fuprintf(stderr, "  -of FNAME, --output-file FNAME [%-7s] output file path (without file extension)\n",      ConvertUTF8toUTF16("").c_str());
    fuprintf(stderr, "  -ps,       --print-special     [%-7s] print special tokens\n",                           bool2ustr(params.print_special).c_str());
    fuprintf(stderr, "  -pc,       --print-colors      [%-7s] print colors\n",                                   bool2ustr(params.print_colors).c_str());
    fuprintf(stderr, "  -pp,       --print-progress    [%-7s] print progress\n",                                 bool2ustr(params.print_progress).c_str());
    fuprintf(stderr, "  -nt,       --no-timestamps     [%-7s] do not print timestamps\n",                        bool2ustr(params.no_timestamps).c_str());
    fuprintf(stderr, "  -l LANG,   --language LANG     [%-7s] spoken language ('auto' for auto-detect)\n",       ConvertUTF8toUTF16(params.language).c_str());
    fuprintf(stderr, "  -dl,       --detect-language   [%-7s] exit after automatically detecting language\n",    bool2ustr(params.detect_language).c_str());
    fuprintf(stderr, "             --prompt PROMPT     [%-7s] initial prompt\n",                                 ConvertUTF8toUTF16(params.prompt).c_str());
    fuprintf(stderr, "  -m FNAME,  --model FNAME       [%-7s] model path\n",                                     ConvertUTF8toUTF16(params.model).c_str());
    fuprintf(stderr, "  -f FNAME,  --file FNAME        [%-7s] input WAV file path\n",                            ConvertUTF8toUTF16("").c_str());
    fuprintf(stderr, "  -oved D,   --ov-e-device DNAME [%-7s] the OpenVINO device used for encode inference\n",  ConvertUTF8toUTF16(params.openvino_encode_device).c_str());
    fuprintf(stderr, "  -ls,       --log-score         [%-7s] log best decoder scores of tokens\n",              bool2ustr(params.log_score).c_str());
    fuprintf(stderr, "\n");
}

struct whisper_print_user_data {
    const whisper_params * params;

    const std::vector<std::vector<float>> * pcmf32s;
    int progress_prev;
};

std::string estimate_diarization_speaker(std::vector<std::vector<float>> pcmf32s, int64_t t0, int64_t t1, bool id_only = false) {
    std::string speaker = "";
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
        speaker = "0";
    } else if (energy1 > 1.1*energy0) {
        speaker = "1";
    } else {
        speaker = "?";
    }

    //uprintf("is0 = %lld, is1 = %lld, energy0 = %f, energy1 = %f, speaker = %s\n", is0, is1, energy0, energy1, speaker.c_str());

    if (!id_only) {
        speaker.insert(0, "(speaker ");
        speaker.append(")");
    }

    return speaker;
}
void whisper_print_progress_callback(struct whisper_context * /*ctx*/, struct whisper_state * /*state*/, int progress, void * user_data) {
    int progress_step = ((whisper_print_user_data *) user_data)->params->progress_step;
    int * progress_prev  = &(((whisper_print_user_data *) user_data)->progress_prev);
    if (progress >= *progress_prev + progress_step) {
        *progress_prev += progress_step;
        fuprintf(stderr, "%s: progress = %3d%%\n", ConvertUTF8toUTF16(__func__).c_str(), progress);
    }
}

void whisper_print_segment_callback(struct whisper_context * ctx, struct whisper_state * /*state*/, int n_new, void * user_data) {
    const auto & params  = *((whisper_print_user_data *) user_data)->params;
    const auto & pcmf32s = *((whisper_print_user_data *) user_data)->pcmf32s;

    const int n_segments = whisper_full_n_segments(ctx);

    std::string speaker = "";

    int64_t t0 = 0;
    int64_t t1 = 0;

    // print the last n_new segments
    const int s0 = n_segments - n_new;

    if (s0 == 0) {
        uprintf("\n");
    }

    for (int i = s0; i < n_segments; i++) {
        if (!params.no_timestamps || params.diarize) {
            t0 = whisper_full_get_segment_t0(ctx, i);
            t1 = whisper_full_get_segment_t1(ctx, i);
        }

        if (!params.no_timestamps) {
            uprintf("[%s --> %s]  ", ConvertUTF8toUTF16(to_timestamp(t0)).c_str(), ConvertUTF8toUTF16(to_timestamp(t1)).c_str());
        }

        if (params.diarize && pcmf32s.size() == 2) {
            speaker = estimate_diarization_speaker(pcmf32s, t0, t1);
        }

        if (params.print_colors) {
            #if WIN32
                std::string ending = "\x1b[0m";
            #else
                std::string ending = "\033[0m";
            #endif
            for (int j = 0; j < whisper_full_n_tokens(ctx, i); ++j) {
                if (params.print_special == false) {
                    const whisper_token id = whisper_full_get_token_id(ctx, i, j);
                    if (id >= whisper_token_eot(ctx)) {
                        continue;
                    }
                }

                const char * text = whisper_full_get_token_text(ctx, i, j);
                const float  p    = whisper_full_get_token_p   (ctx, i, j);

                const int col = std::max(0, std::min((int) k_colors.size() - 1, (int) (std::pow(p, 3)*float(k_colors.size()))));

                uprintf("%s%s%s%s", ConvertUTF8toUTF16(speaker).c_str(), ConvertUTF8toUTF16(k_colors[col]).c_str(), ConvertUTF8toUTF16(text).c_str(), ConvertUTF8toUTF16(ending).c_str());
            }
        } else {
            const char * text = whisper_full_get_segment_text(ctx, i);
            uprintf("%s%s", ConvertUTF8toUTF16(speaker).c_str(), ConvertUTF8toUTF16(text).c_str());
        }

        if (params.tinydiarize) {
            if (whisper_full_get_segment_speaker_turn_next(ctx, i)) {
                uprintf("%s", ConvertUTF8toUTF16(params.tdrz_speaker_turn).c_str());
            }
        }

        // with timestamps or speakers: each segment on new line
        if (!params.no_timestamps || params.diarize) {
            uprintf("\n");
        }

        fflush(stdout);
    }
}

bool output_txt(struct whisper_context * ctx, const char * fname, const whisper_params & params, std::vector<std::vector<float>> pcmf32s) {
    std::ofstream fout(ConvertUTF8toUTF16(fname));
    if (!fout.is_open()) {
        fuprintf(stderr, "%s: failed to open '%s' for writing\n", ConvertUTF8toUTF16(__func__).c_str(), ConvertUTF8toUTF16(fname).c_str());
        return false;
    }

    fuprintf(stderr, "%s: saving output to '%s'\n", ConvertUTF8toUTF16(__func__).c_str(), ConvertUTF8toUTF16(fname).c_str());

    const int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) {
        const char * text = whisper_full_get_segment_text(ctx, i);
        std::string speaker = "";

        if (params.diarize && pcmf32s.size() == 2)
        {
            const int64_t t0 = whisper_full_get_segment_t0(ctx, i);
            const int64_t t1 = whisper_full_get_segment_t1(ctx, i);
            speaker = estimate_diarization_speaker(pcmf32s, t0, t1);
        }

        fout << speaker << text << "\n";
    }

    return true;
}

bool output_vtt(struct whisper_context * ctx, const char * fname, const whisper_params & params, std::vector<std::vector<float>> pcmf32s) {
    std::ofstream fout(ConvertUTF8toUTF16(fname));
    if (!fout.is_open()) {
        fuprintf(stderr, "%s: failed to open '%s' for writing\n", ConvertUTF8toUTF16(__func__).c_str(), ConvertUTF8toUTF16(fname).c_str());
        return false;
    }

    fuprintf(stderr, "%s: saving output to '%s'\n", ConvertUTF8toUTF16(__func__).c_str(), ConvertUTF8toUTF16(fname).c_str());

    fout << "WEBVTT\n\n";

    const int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) {
        const char * text = whisper_full_get_segment_text(ctx, i);
        const int64_t t0 = whisper_full_get_segment_t0(ctx, i);
        const int64_t t1 = whisper_full_get_segment_t1(ctx, i);
        std::string speaker = "";

        if (params.diarize && pcmf32s.size() == 2)
        {
            speaker = estimate_diarization_speaker(pcmf32s, t0, t1, true);
            speaker.insert(0, "<v Speaker");
            speaker.append(">");
        }

        fout << to_timestamp(t0) << " --> " << to_timestamp(t1) << "\n";
        fout << speaker << text << "\n\n";
    }

    return true;
}

bool output_srt(struct whisper_context * ctx, const char * fname, const whisper_params & params, std::vector<std::vector<float>> pcmf32s) {
    std::ofstream fout(ConvertUTF8toUTF16(fname));
    if (!fout.is_open()) {
        fuprintf(stderr, "%s: failed to open '%s' for writing\n", ConvertUTF8toUTF16(__func__).c_str(), ConvertUTF8toUTF16(fname).c_str());
        return false;
    }

    fuprintf(stderr, "%s: saving output to '%s'\n", ConvertUTF8toUTF16(__func__).c_str(), ConvertUTF8toUTF16(fname).c_str());

    const int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) {
        const char * text = whisper_full_get_segment_text(ctx, i);
        const int64_t t0 = whisper_full_get_segment_t0(ctx, i);
        const int64_t t1 = whisper_full_get_segment_t1(ctx, i);
        std::string speaker = "";

        if (params.diarize && pcmf32s.size() == 2)
        {
            speaker = estimate_diarization_speaker(pcmf32s, t0, t1);
        }

        fout << i + 1 + params.offset_n << "\n";
        fout << to_timestamp(t0, true) << " --> " << to_timestamp(t1, true) << "\n";
        fout << speaker << text << "\n\n";
    }

    return true;
}

char *escape_double_quotes_and_backslashes(const char *str) {
    if (str == NULL) {
        return NULL;
    }

    size_t escaped_length = strlen(str) + 1;

    for (size_t i = 0; str[i] != '\0'; i++) {
        if (str[i] == '"' || str[i] == '\\') {
            escaped_length++;
        }
    }

    char *escaped = (char *)calloc(escaped_length, 1); // pre-zeroed
    if (escaped == NULL) {
        return NULL;
    }

    size_t pos = 0;
    for (size_t i = 0; str[i] != '\0'; i++) {
        if (str[i] == '"' || str[i] == '\\') {
            escaped[pos++] = '\\';
        }
        escaped[pos++] = str[i];
    }

    // no need to set zero due to calloc() being used prior

    return escaped;
}

bool output_csv(struct whisper_context * ctx, const char * fname, const whisper_params & params, std::vector<std::vector<float>> pcmf32s) {
    std::ofstream fout(ConvertUTF8toUTF16(fname));
    if (!fout.is_open()) {
        fuprintf(stderr, "%s: failed to open '%s' for writing\n", ConvertUTF8toUTF16(__func__).c_str(), ConvertUTF8toUTF16(fname).c_str());
        return false;
    }

    fuprintf(stderr, "%s: saving output to '%s'\n", ConvertUTF8toUTF16(__func__).c_str(), ConvertUTF8toUTF16(fname).c_str());

    const int n_segments = whisper_full_n_segments(ctx);
    fout << "start,end,";
    if (params.diarize && pcmf32s.size() == 2)
    {
        fout << "speaker,";
    }
    fout << "text\n";

    for (int i = 0; i < n_segments; ++i) {
        const char * text = whisper_full_get_segment_text(ctx, i);
        const int64_t t0 = whisper_full_get_segment_t0(ctx, i);
        const int64_t t1 = whisper_full_get_segment_t1(ctx, i);
        char * text_escaped = escape_double_quotes_and_backslashes(text);

        //need to multiply times returned from whisper_full_get_segment_t{0,1}() by 10 to get milliseconds.
        fout << 10 * t0 << "," << 10 * t1 << ",";
        if (params.diarize && pcmf32s.size() == 2)
        {
            fout << estimate_diarization_speaker(pcmf32s, t0, t1, true) << ",";
        }
        fout << "\"" << text_escaped << "\"\n";
    }

    return true;
}

bool output_score(struct whisper_context * ctx, const char * fname, const whisper_params & /*params*/, std::vector<std::vector<float>> /*pcmf32s*/) {
    std::ofstream fout(ConvertUTF8toUTF16(fname));
    fuprintf(stderr, "%s: saving output to '%s'\n", ConvertUTF8toUTF16(__func__).c_str(), ConvertUTF8toUTF16(fname).c_str());

    const int n_segments = whisper_full_n_segments(ctx);
    // fuprintf(stderr,"segments: %d\n",n_segments);
    for (int i = 0; i < n_segments; ++i) {
        const int n_tokens = whisper_full_n_tokens(ctx, i);
        // fuprintf(stderr,"tokens: %d\n",n_tokens);
        for (int j = 0; j < n_tokens; j++) {
            auto token = whisper_full_get_token_text(ctx, i, j);
            auto probability = whisper_full_get_token_p(ctx, i, j);
            fout << token << '\t' << probability << std::endl;
            // fuprintf(stderr,"token: %s %f\n",token,probability);
	    }
    }
    return true;
}

bool output_json(struct whisper_context * ctx, const char * fname, const whisper_params & params, std::vector<std::vector<float>> pcmf32s) {
    std::ofstream fout(ConvertUTF8toUTF16(fname));
    int indent = 0;

    auto doindent = [&]() {
        for (int i = 0; i < indent; i++) fout << "\t";
    };

    auto start_arr = [&](const char *name) {
        doindent();
        fout << "\"" << name << "\": [\n";
        indent++;
    };

    auto end_arr = [&](bool end) {
        indent--;
        doindent();
        fout << (end ? "]\n" : "},\n");
    };

    auto start_obj = [&](const char *name) {
        doindent();
        if (name) {
            fout << "\"" << name << "\": {\n";
        } else {
            fout << "{\n";
        }
        indent++;
    };

    auto end_obj = [&](bool end) {
        indent--;
        doindent();
        fout << (end ? "}\n" : "},\n");
    };

    auto start_value = [&](const char *name) {
        doindent();
        fout << "\"" << name << "\": ";
    };

    auto value_s = [&](const char *name, const char *val, bool end) {
        start_value(name);
        char * val_escaped = escape_double_quotes_and_backslashes(val);
        fout << "\"" << val_escaped << (end ? "\"\n" : "\",\n");
        free(val_escaped);
    };

    auto end_value = [&](bool end) {
        fout << (end ? "\n" : ",\n");
    };

    auto value_i = [&](const char *name, const int64_t val, bool end) {
        start_value(name);
        fout << val;
        end_value(end);
    };

    auto value_b = [&](const char *name, const bool val, bool end) {
        start_value(name);
        fout << (val ? "true" : "false");
        end_value(end);
    };

    if (!fout.is_open()) {
        fuprintf(stderr, "%s: failed to open '%s' for writing\n", ConvertUTF8toUTF16(__func__).c_str(), ConvertUTF8toUTF16(fname).c_str());
        return false;
    }

    fuprintf(stderr, "%s: saving output to '%s'\n", ConvertUTF8toUTF16(__func__).c_str(), ConvertUTF8toUTF16(fname).c_str());
    start_obj(nullptr);
        value_s("systeminfo", whisper_print_system_info(), false);
        start_obj("model");
            value_s("type", whisper_model_type_readable(ctx), false);
            value_b("multilingual", whisper_is_multilingual(ctx), false);
            value_i("vocab", whisper_model_n_vocab(ctx), false);
            start_obj("audio");
                value_i("ctx", whisper_model_n_audio_ctx(ctx), false);
                value_i("state", whisper_model_n_audio_state(ctx), false);
                value_i("head", whisper_model_n_audio_head(ctx), false);
                value_i("layer", whisper_model_n_audio_layer(ctx), true);
            end_obj(false);
            start_obj("text");
                value_i("ctx", whisper_model_n_text_ctx(ctx), false);
                value_i("state", whisper_model_n_text_state(ctx), false);
                value_i("head", whisper_model_n_text_head(ctx), false);
                value_i("layer", whisper_model_n_text_layer(ctx), true);
            end_obj(false);
            value_i("mels", whisper_model_n_mels(ctx), false);
            value_i("ftype", whisper_model_ftype(ctx), true);
        end_obj(false);
        start_obj("params");
            value_s("model", params.model.c_str(), false);
            value_s("language", params.language.c_str(), false);
            value_b("translate", params.translate, true);
        end_obj(false);
        start_obj("result");
            value_s("language", whisper_lang_str(whisper_full_lang_id(ctx)), true);
        end_obj(false);
        start_arr("transcription");

            const int n_segments = whisper_full_n_segments(ctx);
            for (int i = 0; i < n_segments; ++i) {
                const char * text = whisper_full_get_segment_text(ctx, i);

                const int64_t t0 = whisper_full_get_segment_t0(ctx, i);
                const int64_t t1 = whisper_full_get_segment_t1(ctx, i);

                start_obj(nullptr);
                    start_obj("timestamps");
                        value_s("from", to_timestamp(t0, true).c_str(), false);
                        value_s("to", to_timestamp(t1, true).c_str(), true);
                    end_obj(false);
                    start_obj("offsets");
                        value_i("from", t0 * 10, false);
                        value_i("to", t1 * 10, true);
                    end_obj(false);
                    value_s("text", text, !params.diarize && !params.tinydiarize);

                    if (params.diarize && pcmf32s.size() == 2) {
                        value_s("speaker", estimate_diarization_speaker(pcmf32s, t0, t1, true).c_str(), true);
                    }

                    if (params.tinydiarize) {
                        value_b("speaker_turn_next", whisper_full_get_segment_speaker_turn_next(ctx, i), true);
                    }
                end_obj(i == (n_segments - 1));
            }

        end_arr(true);
    end_obj(true);
    return true;
}

// karaoke video generation
// outputs a bash script that uses ffmpeg to generate a video with the subtitles
// TODO: font parameter adjustments
bool output_wts(struct whisper_context * ctx, const char * fname, const char * fname_inp, const whisper_params & params, float t_sec, std::vector<std::vector<float>> pcmf32s) {
    std::ofstream fout(ConvertUTF8toUTF16(fname));

    fuprintf(stderr, "%s: saving output to '%s'\n", ConvertUTF8toUTF16(__func__).c_str(), ConvertUTF8toUTF16(fname).c_str());

    static const char * font = params.font_path.c_str();

    std::ifstream fin(font);
    if (!fin.is_open()) {
        fuprintf(stderr, "%s: font not found at '%s', please specify a monospace font with -fp\n", ConvertUTF8toUTF16(__func__).c_str(), ConvertUTF8toUTF16(font).c_str());
        return false;
    }

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
        std::string speaker = "";

        if (params.diarize && pcmf32s.size() == 2) {
            speaker = estimate_diarization_speaker(pcmf32s, t0, t1);
        }

        for (int j = 0; j < n; ++j) {
            const auto & token = tokens[j];

            if (tokens[j].id >= whisper_token_eot(ctx)) {
                continue;
            }

            std::string txt_bg = "";
            std::string txt_fg = ""; // highlight token
            std::string txt_ul = ""; // underline

            if (params.diarize && pcmf32s.size() == 2) {
                txt_bg = speaker;
                txt_fg = speaker;
                txt_ul = "\\ \\ \\ \\ \\ \\ \\ \\ \\ \\ \\ ";
            }

            txt_bg.append("> ");
            txt_fg.append("> ");
            txt_ul.append("\\ \\ ");

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

    fuprintf(stderr, "%s: run 'source %s' to generate karaoke video\n", ConvertUTF8toUTF16(__func__).c_str(), ConvertUTF8toUTF16(fname).c_str());

    return true;
}

bool output_lrc(struct whisper_context * ctx, const char * fname, const whisper_params & params, std::vector<std::vector<float>> pcmf32s) {
    std::ofstream fout(ConvertUTF8toUTF16(fname));
    if (!fout.is_open()) {
        fuprintf(stderr, "%s: failed to open '%s' for writing\n", ConvertUTF8toUTF16(__func__).c_str(), ConvertUTF8toUTF16(fname).c_str());
        return false;
    }

    fuprintf(stderr, "%s: saving output to '%s'\n", ConvertUTF8toUTF16(__func__).c_str(), ConvertUTF8toUTF16(fname).c_str());

    fout << "[by:whisper.cpp]\n";

    const int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) {
        const char * text = whisper_full_get_segment_text(ctx, i);
        const int64_t t = whisper_full_get_segment_t0(ctx, i);

        int64_t msec = t * 10;
        int64_t min = msec / (1000 * 60);
        msec = msec - min * (1000 * 60);
        int64_t sec = msec / 1000;
        msec = msec - sec * 1000;

        char buf[16];
        snprintf(buf, sizeof(buf), "%02d:%02d.%02d", (int) min, (int) sec, (int) ( msec / 10));
        std::string timestamp_lrc = std::string(buf);
        std::string speaker = "";

        if (params.diarize && pcmf32s.size() == 2)
        {
            const int64_t t0 = whisper_full_get_segment_t0(ctx, i);
            const int64_t t1 = whisper_full_get_segment_t1(ctx, i);
            speaker = estimate_diarization_speaker(pcmf32s, t0, t1);
        }

        fout <<  '[' << timestamp_lrc << ']' << speaker << text << "\n";
    }

    return true;
}

int init(std::vector<ustring> & args) {
    whisper_params params;

    if (whisper_params_parse(args, params) == false) {
        whisper_print_usage(args, params);
        return 1;
    }

    if (params.fname_inp.empty()) {
        fuprintf(stderr, "error: no input files specified\n");
        whisper_print_usage(args, params);
        return 2;
    }

    if (params.language != "auto" && whisper_lang_id(params.language.c_str()) == -1) {
        fuprintf(stderr, "error: unknown language '%s'\n", ConvertUTF8toUTF16(params.language).c_str());
        whisper_print_usage(args, params);
        exit(0);
    }

    if (params.diarize && params.tinydiarize) {
        fuprintf(stderr, "error: cannot use both --diarize and --tinydiarize\n");
        whisper_print_usage(args, params);
        exit(0);
    }

    // whisper init
    struct whisper_context * ctx = whisper_init_from_file(params.model.c_str());

    if (ctx == nullptr) {
        fuprintf(stderr, "error: failed to initialize whisper context\n");
        return 3;
    }

    // initialize openvino encoder. this has no effect on whisper.cpp builds that don't have OpenVINO configured
    whisper_ctx_init_openvino_encoder(ctx, nullptr, params.openvino_encode_device.c_str(), nullptr);

    for (int f = 0; f < (int) params.fname_inp.size(); ++f) {
        const auto fname_inp = params.fname_inp[f];
        const auto fname_out = f < (int) params.fname_out.size() && !params.fname_out[f].empty() ? params.fname_out[f] : params.fname_inp[f];

        std::vector<float> pcmf32;               // mono-channel F32 PCM
        std::vector<std::vector<float>> pcmf32s; // stereo-channel F32 PCM

        if (!::read_wav(fname_inp, pcmf32, pcmf32s, params.diarize)) {
            fuprintf(stderr, "error: failed to read WAV file '%s'\n", ConvertUTF8toUTF16(fname_inp).c_str());
            continue;
        }

        // print system information
        {
            fuprintf(stderr, "\n");
            fuprintf(stderr, "system_info: n_threads = %d / %d | %s\n",
                     params.n_threads*params.n_processors, std::thread::hardware_concurrency(), ConvertUTF8toUTF16(whisper_print_system_info()).c_str());
        }

        // print some info about the processing
        {
            fuprintf(stderr, "\n");
            if (!whisper_is_multilingual(ctx)) {
                if (params.language != "en" || params.translate) {
                    params.language = "en";
                    params.translate = false;
                    fuprintf(stderr, "%s: WARNING: model is not multilingual, ignoring language and translation options\n", __func__);
                }
            }
            if (params.detect_language) {
                params.language = "auto";
            }
            fuprintf(stderr, "%s: processing '%s' (%d samples, %.1f sec), %d threads, %d processors, lang = %s, task = %s, %stimestamps = %d ...\n",
                     ConvertUTF8toUTF16(__func__).c_str(), ConvertUTF8toUTF16(fname_inp).c_str(), int(pcmf32.size()), float(pcmf32.size())/WHISPER_SAMPLE_RATE,
                     params.n_threads, params.n_processors,
                     ConvertUTF8toUTF16(params.language).c_str(),
                     bool2ustr(params.translate).c_str(),
                     bool2ustr(params.tinydiarize, "tdrz = 1, ", "").c_str(),
                     params.no_timestamps ? 0 : 1);

            fuprintf(stderr, "\n");
        }

        // run the inference
        {
            whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);

            wparams.strategy = params.beam_size > 1 ? WHISPER_SAMPLING_BEAM_SEARCH : WHISPER_SAMPLING_GREEDY;

            wparams.print_realtime   = false;
            wparams.print_progress   = params.print_progress;
            wparams.print_timestamps = !params.no_timestamps;
            wparams.print_special    = params.print_special;
            wparams.translate        = params.translate;
            wparams.language         = params.language.c_str();
            wparams.detect_language  = params.detect_language;
            wparams.n_threads        = params.n_threads;
            wparams.n_max_text_ctx   = params.max_context >= 0 ? params.max_context : wparams.n_max_text_ctx;
            wparams.offset_ms        = params.offset_t_ms;
            wparams.duration_ms      = params.duration_ms;

            wparams.token_timestamps = params.output_wts || params.max_len > 0;
            wparams.thold_pt         = params.word_thold;
            wparams.max_len          = params.output_wts && params.max_len == 0 ? 60 : params.max_len;
            wparams.split_on_word    = params.split_on_word;

            wparams.speed_up         = params.speed_up;
            wparams.debug_mode       = params.debug_mode;

            wparams.tdrz_enable      = params.tinydiarize; // [TDRZ]

            wparams.initial_prompt   = params.prompt.c_str();

            wparams.greedy.best_of        = params.best_of;
            wparams.beam_search.beam_size = params.beam_size;

            wparams.temperature_inc  = params.no_fallback ? 0.0f : wparams.temperature_inc;
            wparams.entropy_thold    = params.entropy_thold;
            wparams.logprob_thold    = params.logprob_thold;

            whisper_print_user_data user_data = { &params, &pcmf32s, 0 };

            // this callback is called on each new segment
            if (!wparams.print_realtime) {
                wparams.new_segment_callback           = whisper_print_segment_callback;
                wparams.new_segment_callback_user_data = &user_data;
            }

            if (wparams.print_progress) {
                wparams.progress_callback           = whisper_print_progress_callback;
                wparams.progress_callback_user_data = &user_data;
            }

            // example for abort mechanism
            // in this example, we do not abort the processing, but we could if the flag is set to true
            // the callback is called before every encoder run - if it returns false, the processing is aborted
            {
                static bool is_aborted = false; // NOTE: this should be atomic to avoid data race

                wparams.encoder_begin_callback = [](struct whisper_context * /*ctx*/, struct whisper_state * /*state*/, void * user_data) {
                    bool is_aborted = *(bool*)user_data;
                    return !is_aborted;
                };
                wparams.encoder_begin_callback_user_data = &is_aborted;
            }

            if (whisper_full_parallel(ctx, wparams, pcmf32.data(), pcmf32.size(), params.n_processors) != 0) {
                fuprintf(stderr, "%s: failed to process audio\n", args[0].c_str());
                return 10;
            }
        }

        // output stuff
        {
            uprintf("\n");

            // output to text file
            if (params.output_txt) {
                const auto fname_txt = fname_out + ".txt";
                output_txt(ctx, fname_txt.c_str(), params, pcmf32s);
            }

            // output to VTT file
            if (params.output_vtt) {
                const auto fname_vtt = fname_out + ".vtt";
                output_vtt(ctx, fname_vtt.c_str(), params, pcmf32s);
            }

            // output to SRT file
            if (params.output_srt) {
                const auto fname_srt = fname_out + ".srt";
                output_srt(ctx, fname_srt.c_str(), params, pcmf32s);
            }

            // output to WTS file
            if (params.output_wts) {
                const auto fname_wts = fname_out + ".wts";
                output_wts(ctx, fname_wts.c_str(), fname_inp.c_str(), params, float(pcmf32.size() + 1000)/WHISPER_SAMPLE_RATE, pcmf32s);
            }

            // output to CSV file
            if (params.output_csv) {
                const auto fname_csv = fname_out + ".csv";
                output_csv(ctx, fname_csv.c_str(), params, pcmf32s);
            }

            // output to JSON file
            if (params.output_jsn) {
                const auto fname_jsn = fname_out + ".json";
                output_json(ctx, fname_jsn.c_str(), params, pcmf32s);
            }

            // output to LRC file
            if (params.output_lrc) {
                const auto fname_lrc = fname_out + ".lrc";
                output_lrc(ctx, fname_lrc.c_str(), params, pcmf32s);
            }

            // output to score file
            if (params.log_score) {
                const auto fname_score = fname_out + ".score.txt";
                output_score(ctx, fname_score.c_str(), params, pcmf32s);
            }
        }
    }

    whisper_print_timings(ctx);
    whisper_free(ctx);

    return 0;
}

#if WIN32
int wmain(int argc, wchar_t ** argv) {
    init_console();
    std::vector<ustring> args(argc);
    for (int i = 0; i < argc; i++) {
        args[i] = argv[i];
    }
    init(args);
}
#else
int main(int argc, char ** argv) {
    init_console();
    std::vector<ustring> args(argc);
    for (int i = 0; i < argc; i++) {
        args[i] = std::string(argv[i]);
    }
    init(args);
}
#endif
