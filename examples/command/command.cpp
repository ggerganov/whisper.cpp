// Voice assistant example
//
// Speak short text commands to the microphone.
// This program will detect your voice command and convert them to text.
//
// ref: https://github.com/ggerganov/whisper.cpp/issues/171
//

#include "whisper.h"

#include <SDL.h>
#include <SDL_audio.h>

#include <sstream>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <mutex>
#include <regex>
#include <string>
#include <thread>
#include <vector>
#include <map>

// command-line parameters
struct whisper_params {
    int32_t n_threads  = std::min(4, (int32_t) std::thread::hardware_concurrency());
    int32_t prompt_ms  = 5000;
    int32_t command_ms = 8000;
    int32_t capture_id = -1;
    int32_t max_tokens = 32;
    int32_t audio_ctx  = 0;

    float vad_thold    = 0.6f;
    float freq_thold   = 100.0f;

    bool speed_up      = false;
    bool translate     = false;
    bool print_special = false;
    bool print_energy  = false;
    bool no_timestamps = true;

    std::string language  = "en";
    std::string model     = "models/ggml-base.en.bin";
    std::string fname_out;
    std::string commands;
    std::string prompt;
};

void whisper_print_usage(int argc, char ** argv, const whisper_params & params);

bool whisper_params_parse(int argc, char ** argv, whisper_params & params) {
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            whisper_print_usage(argc, argv, params);
            exit(0);
        }
        else if (arg == "-t"   || arg == "--threads")       { params.n_threads     = std::stoi(argv[++i]); }
        else if (arg == "-pms" || arg == "--prompt-ms")     { params.prompt_ms     = std::stoi(argv[++i]); }
        else if (arg == "-cms" || arg == "--command-ms")    { params.command_ms    = std::stoi(argv[++i]); }
        else if (arg == "-c"   || arg == "--capture")       { params.capture_id    = std::stoi(argv[++i]); }
        else if (arg == "-mt"  || arg == "--max-tokens")    { params.max_tokens    = std::stoi(argv[++i]); }
        else if (arg == "-ac"  || arg == "--audio-ctx")     { params.audio_ctx     = std::stoi(argv[++i]); }
        else if (arg == "-vth" || arg == "--vad-thold")     { params.vad_thold     = std::stof(argv[++i]); }
        else if (arg == "-fth" || arg == "--freq-thold")    { params.freq_thold    = std::stof(argv[++i]); }
        else if (arg == "-su"  || arg == "--speed-up")      { params.speed_up      = true; }
        else if (arg == "-tr"  || arg == "--translate")     { params.translate     = true; }
        else if (arg == "-ps"  || arg == "--print-special") { params.print_special = true; }
        else if (arg == "-pe"  || arg == "--print-energy")  { params.print_energy  = true; }
        else if (arg == "-l"   || arg == "--language")      { params.language      = argv[++i]; }
        else if (arg == "-m"   || arg == "--model")         { params.model         = argv[++i]; }
        else if (arg == "-f"   || arg == "--file")          { params.fname_out     = argv[++i]; }
        else if (arg == "-cmd" || arg == "--commands")      { params.commands      = argv[++i]; }
        else if (arg == "-p"   || arg == "--prompt")        { params.prompt        = argv[++i]; }
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
    fprintf(stderr, "  -h,         --help           [default] show this help message and exit\n");
    fprintf(stderr, "  -t N,       --threads N      [%-7d] number of threads to use during computation\n", params.n_threads);
    fprintf(stderr, "  -pms N,     --prompt-ms N    [%-7d] prompt duration in milliseconds\n",             params.prompt_ms);
    fprintf(stderr, "  -cms N,     --command-ms N   [%-7d] command duration in milliseconds\n",            params.command_ms);
    fprintf(stderr, "  -c ID,      --capture ID     [%-7d] capture device ID\n",                           params.capture_id);
    fprintf(stderr, "  -mt N,      --max-tokens N   [%-7d] maximum number of tokens per audio chunk\n",    params.max_tokens);
    fprintf(stderr, "  -ac N,      --audio-ctx N    [%-7d] audio context size (0 - all)\n",                params.audio_ctx);
    fprintf(stderr, "  -vth N,     --vad-thold N    [%-7.2f] voice activity detection threshold\n",        params.vad_thold);
    fprintf(stderr, "  -fth N,     --freq-thold N   [%-7.2f] high-pass frequency cutoff\n",                params.freq_thold);
    fprintf(stderr, "  -su,        --speed-up       [%-7s] speed up audio by x2 (reduced accuracy)\n",     params.speed_up ? "true" : "false");
    fprintf(stderr, "  -tr,        --translate      [%-7s] translate from source language to english\n",   params.translate ? "true" : "false");
    fprintf(stderr, "  -ps,        --print-special  [%-7s] print special tokens\n",                        params.print_special ? "true" : "false");
    fprintf(stderr, "  -pe,        --print-energy   [%-7s] print sound energy (for debugging)\n",          params.print_energy ? "true" : "false");
    fprintf(stderr, "  -l LANG,    --language LANG  [%-7s] spoken language\n",                             params.language.c_str());
    fprintf(stderr, "  -m FNAME,   --model FNAME    [%-7s] model path\n",                                  params.model.c_str());
    fprintf(stderr, "  -f FNAME,   --file FNAME     [%-7s] text output file name\n",                       params.fname_out.c_str());
    fprintf(stderr, "  -cmd FNAME, --commands FNAME [%-7s] text file with allowed commands\n",             params.commands.c_str());
    fprintf(stderr, "  -p,         --prompt         [%-7s] the required activation prompt\n",              params.prompt.c_str());
    fprintf(stderr, "\n");
}

//
// SDL Audio capture
//

class audio_async {
public:
    audio_async(int len_ms);
    ~audio_async();

    bool init(int capture_id, int sample_rate);

    // start capturing audio via the provided SDL callback
    // keep last len_ms seconds of audio in a circular buffer
    bool resume();
    bool pause();
    bool clear();

    // callback to be called by SDL
    void callback(uint8_t * stream, int len);

    // get audio data from the circular buffer
    void get(int ms, std::vector<float> & audio);

private:
    SDL_AudioDeviceID m_dev_id_in = 0;

    int m_len_ms = 0;
    int m_sample_rate = 0;

    bool       m_running = false;
    std::mutex m_mutex;

    std::vector<float> m_audio;
    std::vector<float> m_audio_new;
    size_t             m_audio_pos = 0;
    size_t             m_audio_len = 0;
};

audio_async::audio_async(int len_ms) {
    m_len_ms = len_ms;
}

audio_async::~audio_async() {
    if (m_dev_id_in) {
        SDL_CloseAudioDevice(m_dev_id_in);
    }
}

bool audio_async::init(int capture_id, int sample_rate) {
    SDL_LogSetPriority(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO);

    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Couldn't initialize SDL: %s\n", SDL_GetError());
        return false;
    }

    SDL_SetHintWithPriority(SDL_HINT_AUDIO_RESAMPLING_MODE, "medium", SDL_HINT_OVERRIDE);

    {
        int nDevices = SDL_GetNumAudioDevices(SDL_TRUE);
        fprintf(stderr, "%s: found %d capture devices:\n", __func__, nDevices);
        for (int i = 0; i < nDevices; i++) {
            fprintf(stderr, "%s:    - Capture device #%d: '%s'\n", __func__, i, SDL_GetAudioDeviceName(i, SDL_TRUE));
        }
    }

    SDL_AudioSpec capture_spec_requested;
    SDL_AudioSpec capture_spec_obtained;

    SDL_zero(capture_spec_requested);
    SDL_zero(capture_spec_obtained);

    capture_spec_requested.freq     = sample_rate;
    capture_spec_requested.format   = AUDIO_F32;
    capture_spec_requested.channels = 1;
    capture_spec_requested.samples  = 1024;
    capture_spec_requested.callback = [](void * userdata, uint8_t * stream, int len) {
        audio_async * audio = (audio_async *) userdata;
        audio->callback(stream, len);
    };
    capture_spec_requested.userdata = this;

    if (capture_id >= 0) {
        fprintf(stderr, "%s: attempt to open capture device %d : '%s' ...\n", __func__, capture_id, SDL_GetAudioDeviceName(capture_id, SDL_TRUE));
        m_dev_id_in = SDL_OpenAudioDevice(SDL_GetAudioDeviceName(capture_id, SDL_TRUE), SDL_TRUE, &capture_spec_requested, &capture_spec_obtained, 0);
    } else {
        fprintf(stderr, "%s: attempt to open default capture device ...\n", __func__);
        m_dev_id_in = SDL_OpenAudioDevice(nullptr, SDL_TRUE, &capture_spec_requested, &capture_spec_obtained, 0);
    }

    if (!m_dev_id_in) {
        fprintf(stderr, "%s: couldn't open an audio device for capture: %s!\n", __func__, SDL_GetError());
        m_dev_id_in = 0;

        return false;
    } else {
        fprintf(stderr, "%s: obtained spec for input device (SDL Id = %d):\n", __func__, m_dev_id_in);
        fprintf(stderr, "%s:     - sample rate:       %d\n",                   __func__, capture_spec_obtained.freq);
        fprintf(stderr, "%s:     - format:            %d (required: %d)\n",    __func__, capture_spec_obtained.format,
                capture_spec_requested.format);
        fprintf(stderr, "%s:     - channels:          %d (required: %d)\n",    __func__, capture_spec_obtained.channels,
                capture_spec_requested.channels);
        fprintf(stderr, "%s:     - samples per frame: %d\n",                   __func__, capture_spec_obtained.samples);
    }

    m_sample_rate = capture_spec_obtained.freq;

    m_audio.resize((m_sample_rate*m_len_ms)/1000);

    return true;
}

bool audio_async::resume() {
    if (!m_dev_id_in) {
        fprintf(stderr, "%s: no audio device to resume!\n", __func__);
        return false;
    }

    if (m_running) {
        fprintf(stderr, "%s: already running!\n", __func__);
        return false;
    }

    SDL_PauseAudioDevice(m_dev_id_in, 0);

    m_running = true;

    return true;
}

bool audio_async::pause() {
    if (!m_dev_id_in) {
        fprintf(stderr, "%s: no audio device to pause!\n", __func__);
        return false;
    }

    if (!m_running) {
        fprintf(stderr, "%s: already paused!\n", __func__);
        return false;
    }

    SDL_PauseAudioDevice(m_dev_id_in, 1);

    m_running = false;

    return true;
}

bool audio_async::clear() {
    if (!m_dev_id_in) {
        fprintf(stderr, "%s: no audio device to clear!\n", __func__);
        return false;
    }

    if (!m_running) {
        fprintf(stderr, "%s: not running!\n", __func__);
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        m_audio_pos = 0;
        m_audio_len = 0;
    }

    return true;
}

// callback to be called by SDL
void audio_async::callback(uint8_t * stream, int len) {
    if (!m_running) {
        return;
    }

    const size_t n_samples = len / sizeof(float);

    m_audio_new.resize(n_samples);
    memcpy(m_audio_new.data(), stream, n_samples * sizeof(float));

    //fprintf(stderr, "%s: %zu samples, pos %zu, len %zu\n", __func__, n_samples, m_audio_pos, m_audio_len);

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_audio_pos + n_samples > m_audio.size()) {
            const size_t n0 = m_audio.size() - m_audio_pos;

            memcpy(&m_audio[m_audio_pos], stream, n0 * sizeof(float));
            memcpy(&m_audio[0], &stream[n0], (n_samples - n0) * sizeof(float));

            m_audio_pos = (m_audio_pos + n_samples) % m_audio.size();
            m_audio_len = m_audio.size();
        } else {
            memcpy(&m_audio[m_audio_pos], stream, n_samples * sizeof(float));

            m_audio_pos = (m_audio_pos + n_samples) % m_audio.size();
            m_audio_len = std::min(m_audio_len + n_samples, m_audio.size());
        }
    }
}

void audio_async::get(int ms, std::vector<float> & result) {
    if (!m_dev_id_in) {
        fprintf(stderr, "%s: no audio device to get audio from!\n", __func__);
        return;
    }

    if (!m_running) {
        fprintf(stderr, "%s: not running!\n", __func__);
        return;
    }

    result.clear();

    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (ms <= 0) {
            ms = m_len_ms;
        }

        size_t n_samples = (m_sample_rate * ms) / 1000;
        if (n_samples > m_audio_len) {
            n_samples = m_audio_len;
        }

        result.resize(n_samples);

        int s0 = m_audio_pos - n_samples;
        if (s0 < 0) {
            s0 += m_audio.size();
        }

        if (s0 + n_samples > m_audio.size()) {
            const size_t n0 = m_audio.size() - s0;

            memcpy(result.data(), &m_audio[s0], n0 * sizeof(float));
            memcpy(&result[n0], &m_audio[0], (n_samples - n0) * sizeof(float));
        } else {
            memcpy(result.data(), &m_audio[s0], n_samples * sizeof(float));
        }
    }
}

///////////////////////////

std::string trim(const std::string & s) {
    std::regex e("^\\s+|\\s+$");
    return std::regex_replace(s, e, "");
}

void high_pass_filter(std::vector<float> & data, float cutoff, float sample_rate) {
    const float rc = 1.0f / (2.0f * M_PI * cutoff);
    const float dt = 1.0f / sample_rate;
    const float alpha = dt / (rc + dt);

    float y = data[0];

    for (size_t i = 1; i < data.size(); i++) {
        y = alpha * (y + data[i] - data[i - 1]);
        data[i] = y;
    }
}

bool vad_simple(std::vector<float> & pcmf32, int sample_rate, int last_ms, float vad_thold, float freq_thold, bool verbose) {
    const int n_samples      = pcmf32.size();
    const int n_samples_last = (sample_rate * last_ms) / 1000;

    if (n_samples_last >= n_samples) {
        // not enough samples - assume no speech
        return false;
    }

    if (freq_thold > 0.0f) {
        high_pass_filter(pcmf32, freq_thold, sample_rate);
    }

    float energy_all  = 0.0f;
    float energy_last = 0.0f;

    for (int i = 0; i < n_samples; i++) {
        energy_all += fabsf(pcmf32[i]);

        if (i >= n_samples - n_samples_last) {
            energy_last += fabsf(pcmf32[i]);
        }
    }

    energy_all  /= n_samples;
    energy_last /= n_samples_last;

    if (verbose) {
        fprintf(stderr, "%s: energy_all: %f, energy_last: %f, vad_thold: %f, freq_thold: %f\n", __func__, energy_all, energy_last, vad_thold, freq_thold);
    }

    if (energy_last > vad_thold*energy_all) {
        return false;
    }

    return true;
}

std::string transcribe(whisper_context * ctx, const whisper_params & params, const std::vector<float> & pcmf32, float & prob, int64_t & t_ms) {
    const auto t_start = std::chrono::high_resolution_clock::now();

    prob = 0.0f;
    t_ms = 0;

    whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);

    wparams.print_progress   = false;
    wparams.print_special    = params.print_special;
    wparams.print_realtime   = false;
    wparams.print_timestamps = !params.no_timestamps;
    wparams.translate        = params.translate;
    wparams.no_context       = true;
    wparams.single_segment   = true;
    wparams.max_tokens       = params.max_tokens;
    wparams.language         = params.language.c_str();
    wparams.n_threads        = params.n_threads;

    wparams.audio_ctx        = params.audio_ctx;
    wparams.speed_up         = params.speed_up;

    if (whisper_full(ctx, wparams, pcmf32.data(), pcmf32.size()) != 0) {
        return "";
    }

    int prob_n = 0;
    std::string result;

    const int n_segments = whisper_full_n_segments(ctx);
    for (int i = 0; i < n_segments; ++i) {
        const char * text = whisper_full_get_segment_text(ctx, i);

        result += text;

        const int n_tokens = whisper_full_n_tokens(ctx, i);
        for (int j = 0; j < n_tokens; ++j) {
            const auto token = whisper_full_get_token_data(ctx, i, j);

            prob += token.p;
            ++prob_n;
        }
    }

    if (prob_n > 0) {
        prob /= prob_n;
    }

    const auto t_end = std::chrono::high_resolution_clock::now();
    t_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count();

    return result;
}

// compute similarity between two strings using Levenshtein distance
float similarity(const std::string & s0, const std::string & s1) {
    const size_t len0 = s0.size() + 1;
    const size_t len1 = s1.size() + 1;

    std::vector<int> col(len1, 0);
    std::vector<int> prevCol(len1, 0);

    for (size_t i = 0; i < len1; i++) {
        prevCol[i] = i;
    }

    for (size_t i = 0; i < len0; i++) {
        col[0] = i;
        for (size_t j = 1; j < len1; j++) {
            col[j] = std::min(std::min(1 + col[j - 1], 1 + prevCol[j]), prevCol[j - 1] + (s0[i - 1] == s1[j - 1] ? 0 : 1));
        }
        col.swap(prevCol);
    }

    const float dist = prevCol[len1 - 1];

    return 1.0f - (dist / std::max(s0.size(), s1.size()));
}

std::vector<std::string> read_allowed_commands(const std::string & fname) {
    std::vector<std::string> allowed_commands;

    std::ifstream ifs(fname);
    if (!ifs.is_open()) {
        return allowed_commands;
    }

    std::string line;
    while (std::getline(ifs, line)) {
        line = trim(line);
        if (line.empty()) {
            continue;
        }

        std::transform(line.begin(), line.end(),line.begin(), ::tolower);
        allowed_commands.push_back(std::move(line));
    }

    return allowed_commands;
}

std::vector<std::string> get_words(const std::string &txt) {
    std::vector<std::string> words;

    std::istringstream iss(txt);
    std::string word;
    while (iss >> word) {
        words.push_back(word);
    }

    return words;
}

// returns true if no exit event was received
bool process_sdl_events() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                {
                    return false;
                } break;
            default:
                break;
        }
    }

    return true;
}

// command-list mode
// guide the transcription to match the most likely command from a provided list
int process_command_list(struct whisper_context * ctx, audio_async &audio, const whisper_params &params) {
    fprintf(stderr, "\n");
    fprintf(stderr, "%s: guided mode\n", __func__);

    std::vector<std::string> allowed_commands = read_allowed_commands(params.commands);

    if (allowed_commands.empty()) {
        fprintf(stderr, "%s: error: failed to read allowed commands from '%s'\n", __func__, params.commands.c_str());
        return 2;
    }

    int max_len = 0;

    std::vector<std::vector<whisper_token>> allowed_tokens;

    for (const auto & cmd : allowed_commands) {
        whisper_token tokens[1024];
        allowed_tokens.emplace_back();

        for (int l = 0; l < (int) cmd.size(); ++l) {
            // NOTE: very important to add the whitespace !
            //       the reason is that the first decoded token starts with a whitespace too!
            std::string ss = std::string(" ") + cmd.substr(0, l + 1);

            const int n = whisper_tokenize(ctx, ss.c_str(), tokens, 1024);
            if (n < 0) {
                fprintf(stderr, "%s: error: failed to tokenize command '%s'\n", __func__, cmd.c_str());
                return 3;
            }

            if (n == 1) {
                allowed_tokens.back().push_back(tokens[0]);
            }
        }

        max_len = std::max(max_len, (int) cmd.size());
    }

    fprintf(stderr, "%s: allowed commands [ tokens ]:\n", __func__);
    fprintf(stderr, "\n");
    for (int i = 0; i < (int) allowed_commands.size(); ++i) {
        fprintf(stderr, "  - \033[1m%-*s\033[0m = [", max_len, allowed_commands[i].c_str());
        for (const auto & token : allowed_tokens[i]) {
            fprintf(stderr, " %5d", token);
        }
        fprintf(stderr, " ]\n");
    }

    std::string  k_prompt = "select one from the available words: ";
    for (int i = 0; i < (int) allowed_commands.size(); ++i) {
        if (i > 0) {
            k_prompt += ", ";
        }
        k_prompt += allowed_commands[i];
    }
    k_prompt += ". selected word: ";

    // tokenize prompt
    std::vector<whisper_token> k_tokens;
    {
        k_tokens.resize(1024);
        const int n = whisper_tokenize(ctx, k_prompt.c_str(), k_tokens.data(), 1024);
        if (n < 0) {
            fprintf(stderr, "%s: error: failed to tokenize prompt '%s'\n", __func__, k_prompt.c_str());
            return 4;
        }
        k_tokens.resize(n);
    }

    fprintf(stderr, "\n");
    fprintf(stderr, "%s: prompt: '%s'\n", __func__, k_prompt.c_str());
    fprintf(stderr, "%s: tokens: [", __func__);
    for (const auto & token : k_tokens) {
        fprintf(stderr, " %d", token);
    }
    fprintf(stderr, " ]\n");

    fprintf(stderr, "\n");
    fprintf(stderr, "%s: listening for a command ...\n", __func__);
    fprintf(stderr, "\n");

    bool is_running  = true;

    std::vector<float> pcmf32_cur;
    std::vector<float> pcmf32_prompt;

    // main loop
    while (is_running) {
        // handle Ctrl + C
        is_running = process_sdl_events();

        // delay
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        audio.get(2000, pcmf32_cur);

        if (vad_simple(pcmf32_cur, WHISPER_SAMPLE_RATE, 1000, params.vad_thold, params.freq_thold, params.print_energy)) {
            fprintf(stdout, "%s: Speech detected! Processing ...\n", __func__);

            const auto t_start = std::chrono::high_resolution_clock::now();

            whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);

            wparams.print_progress   = false;
            wparams.print_special    = params.print_special;
            wparams.print_realtime   = false;
            wparams.print_timestamps = !params.no_timestamps;
            wparams.translate        = params.translate;
            wparams.no_context       = true;
            wparams.single_segment   = true;
            wparams.max_tokens       = 1;
            wparams.language         = params.language.c_str();
            wparams.n_threads        = params.n_threads;

            wparams.audio_ctx        = params.audio_ctx;
            wparams.speed_up         = params.speed_up;

            wparams.prompt_tokens    = k_tokens.data();
            wparams.prompt_n_tokens  = k_tokens.size();

            // run the transformer and a single decoding pass
            if (whisper_full(ctx, wparams, pcmf32_cur.data(), pcmf32_cur.size()) != 0) {
                fprintf(stderr, "%s: ERROR: whisper_full() failed\n", __func__);
                break;
            }

            const auto * probs = whisper_get_probs(ctx);
            std::vector<std::pair<float, int>> probs_id;

            double psum = 0.0;
            for (int i = 0; i < (int) allowed_commands.size(); ++i) {
                probs_id.emplace_back(probs[allowed_tokens[i][0]], i);
                for (int j = 1; j < (int) allowed_tokens[i].size(); ++j) {
                    probs_id.back().first += probs[allowed_tokens[i][j]];
                }
                probs_id.back().first /= allowed_tokens[i].size();
                psum += probs_id.back().first;
            }

            // normalize
            for (auto & p : probs_id) {
                p.first /= psum;
            }

            // sort descending
            {
                using pair_type = decltype(probs_id)::value_type;
                std::sort(probs_id.begin(), probs_id.end(), [](const pair_type & a, const pair_type & b) {
                    return a.first > b.first;
                });
            }

            // print the commands and the respective probabilities
            {
                fprintf(stdout, "\n");
                for (const auto & cmd : probs_id) {
                    fprintf(stdout, "%s: %s%-*s%s = %f | ", __func__, "\033[1m", max_len, allowed_commands[cmd.second].c_str(), "\033[0m", cmd.first);
                    for (int token : allowed_tokens[cmd.second]) {
                        fprintf(stdout, "'%4s' %f ", whisper_token_to_str(ctx, token), probs[token]);
                    }
                    fprintf(stdout, "\n");
                }
            }

            // best command
            {
                const auto t_end = std::chrono::high_resolution_clock::now();

                const float prob = probs_id[0].first;
                const int index = probs_id[0].second;

                fprintf(stdout, "\n");
                fprintf(stdout, "%s: detected command: %s%s%s | p = %f | t = %d ms\n", __func__,
                        "\033[1m", allowed_commands[index].c_str(), "\033[0m", prob,
                        (int) std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count());
                fprintf(stdout, "\n");
            }

            audio.clear();
        }
    }

    return 0;
}

// always-prompt mode
// transcribe the voice into text after valid prompt
int always_prompt_transcription(struct whisper_context * ctx, audio_async & audio, const whisper_params & params) {
    bool is_running = true;
    bool ask_prompt = true;

    float prob = 0.0f;

    std::vector<float> pcmf32_cur;

    const std::string k_prompt = params.prompt;

    const int k_prompt_length = get_words(k_prompt).size();

    fprintf(stderr, "\n");
    fprintf(stderr, "%s: always-prompt mode\n", __func__);

    // main loop
    while (is_running) {
        // handle Ctrl + C
        is_running = process_sdl_events();

        // delay
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        if (ask_prompt) {
            fprintf(stdout, "\n");
            fprintf(stdout, "%s: The prompt is: '%s%s%s'\n", __func__, "\033[1m", k_prompt.c_str(), "\033[0m");
            fprintf(stdout, "\n");

            ask_prompt = false;
        }

        {
            audio.get(2000, pcmf32_cur);

            if (vad_simple(pcmf32_cur, WHISPER_SAMPLE_RATE, 1000, params.vad_thold, params.freq_thold, params.print_energy)) {
                fprintf(stdout, "%s: Speech detected! Processing ...\n", __func__);

                int64_t t_ms = 0;

                // detect the commands
                audio.get(params.command_ms, pcmf32_cur);

                const auto txt = ::trim(::transcribe(ctx, params, pcmf32_cur, prob, t_ms));

                const auto words = get_words(txt);

                std::string prompt;
                std::string command;

                for (int i = 0; i < (int) words.size(); ++i) {
                    if (i < k_prompt_length) {
                        prompt += words[i] + " ";
                    } else {
                        command += words[i] + " ";
                    }
                }

                const float sim = similarity(prompt, k_prompt);

                //debug
                //fprintf(stdout, "command size: %i\n", command_length);

                if ((sim > 0.7f) && (command.size() > 0)) {
                    fprintf(stdout, "%s: Command '%s%s%s', (t = %d ms)\n", __func__, "\033[1m", command.c_str(), "\033[0m", (int) t_ms);
                }

                fprintf(stdout, "\n");

                audio.clear();
            }
        }
    }

    return 0;
}

// general-purpose mode
// freely transcribe the voice into text
int process_general_transcription(struct whisper_context * ctx, audio_async &audio, const whisper_params &params) {
    bool is_running  = true;
    bool have_prompt = false;
    bool ask_prompt  = true;

    float prob0 = 0.0f;
    float prob  = 0.0f;

    std::vector<float> pcmf32_cur;
    std::vector<float> pcmf32_prompt;

    const std::string k_prompt = "Ok Whisper, start listening for commands.";

    fprintf(stderr, "\n");
    fprintf(stderr, "%s: general-purpose mode\n", __func__);

    // main loop
    while (is_running) {
        // handle Ctrl + C
        is_running = process_sdl_events();

        // delay
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        if (ask_prompt) {
            fprintf(stdout, "\n");
            fprintf(stdout, "%s: Say the following phrase: '%s%s%s'\n", __func__, "\033[1m", k_prompt.c_str(), "\033[0m");
            fprintf(stdout, "\n");

            ask_prompt = false;
        }

        {
            audio.get(2000, pcmf32_cur);

            if (vad_simple(pcmf32_cur, WHISPER_SAMPLE_RATE, 1000, params.vad_thold, params.freq_thold, params.print_energy)) {
                fprintf(stdout, "%s: Speech detected! Processing ...\n", __func__);

                int64_t t_ms = 0;

                if (!have_prompt) {
                    // wait for activation phrase
                    audio.get(params.prompt_ms, pcmf32_cur);

                    const auto txt = ::trim(::transcribe(ctx, params, pcmf32_cur, prob0, t_ms));

                    fprintf(stdout, "%s: Heard '%s%s%s', (t = %d ms)\n", __func__, "\033[1m", txt.c_str(), "\033[0m", (int) t_ms);

                    const float sim = similarity(txt, k_prompt);

                    if (txt.length() < 0.8*k_prompt.length() || txt.length() > 1.2*k_prompt.length() || sim < 0.8f) {
                        fprintf(stdout, "%s: WARNING: prompt not recognized, try again\n", __func__);
                        ask_prompt = true;
                    } else {
                        fprintf(stdout, "\n");
                        fprintf(stdout, "%s: The prompt has been recognized!\n", __func__);
                        fprintf(stdout, "%s: Waiting for voice commands ...\n", __func__);
                        fprintf(stdout, "\n");

                        // save the audio for the prompt
                        pcmf32_prompt = pcmf32_cur;
                        have_prompt = true;
                    }
                } else {
                    // we have heard the activation phrase, now detect the commands
                    audio.get(params.command_ms, pcmf32_cur);

                    // prepend the prompt audio
                    pcmf32_cur.insert(pcmf32_cur.begin(), pcmf32_prompt.begin(), pcmf32_prompt.end());

                    const auto txt = ::trim(::transcribe(ctx, params, pcmf32_cur, prob, t_ms));

                    prob = 100.0f*(prob - prob0);

                    //fprintf(stdout, "%s: heard '%s'\n", __func__, txt.c_str());

                    // find the prompt in the text
                    float best_sim = 0.0f;
                    size_t best_len = 0;
                    for (int n = 0.8*k_prompt.size(); n <= 1.2*k_prompt.size(); ++n) {
                        const auto prompt = txt.substr(0, n);

                        const float sim = similarity(prompt, k_prompt);

                        //fprintf(stderr, "%s: prompt = '%s', sim = %f\n", __func__, prompt.c_str(), sim);

                        if (sim > best_sim) {
                            best_sim = sim;
                            best_len = n;
                        }
                    }

                    const std::string command = ::trim(txt.substr(best_len));

                    fprintf(stdout, "%s: Command '%s%s%s', (t = %d ms)\n", __func__, "\033[1m", command.c_str(), "\033[0m", (int) t_ms);
                    fprintf(stdout, "\n");
                }

                audio.clear();
            }
        }
    }

    return 0;
}

int main(int argc, char ** argv) {
    whisper_params params;

    if (whisper_params_parse(argc, argv, params) == false) {
        return 1;
    }

    if (whisper_lang_id(params.language.c_str()) == -1) {
        fprintf(stderr, "error: unknown language '%s'\n", params.language.c_str());
        whisper_print_usage(argc, argv, params);
        exit(0);
    }

    // whisper init

    struct whisper_context * ctx = whisper_init_from_file(params.model.c_str());

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
        fprintf(stderr, "%s: processing, %d threads, lang = %s, task = %s, timestamps = %d ...\n",
                __func__,
                params.n_threads,
                params.language.c_str(),
                params.translate ? "translate" : "transcribe",
                params.no_timestamps ? 0 : 1);

        fprintf(stderr, "\n");
    }

    // init audio

    audio_async audio(30*1000);
    if (!audio.init(params.capture_id, WHISPER_SAMPLE_RATE)) {
        fprintf(stderr, "%s: audio.init() failed!\n", __func__);
        return 1;
    }

    audio.resume();

    // wait for 1 second to avoid any buffered noise
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    audio.clear();

    int  ret_val = 0;

    if (!params.commands.empty()) {
        ret_val = process_command_list(ctx, audio, params);
    } else if (!params.prompt.empty()) {
        ret_val = always_prompt_transcription(ctx, audio, params);
    } else {
        ret_val = process_general_transcription(ctx, audio, params);
    }

    audio.pause();

    whisper_print_timings(ctx);
    whisper_free(ctx);

    return ret_val;
}
