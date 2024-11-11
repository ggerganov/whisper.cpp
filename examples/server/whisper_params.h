#pragma once

#include <string>
#include <thread>
#include <algorithm>

namespace whisper {

// output formats
static const std::string json_format   = "json";
static const std::string text_format   = "text";
static const std::string srt_format    = "srt";
static const std::string vjson_format  = "verbose_json";
static const std::string vtt_format    = "vtt";

// Server parameters
struct server_params {
    std::string hostname = "127.0.0.1";
    int port = 8080;
    std::string public_path = ".";
    std::string request_path = "";
    std::string inference_path = "/inference";
    bool ffmpeg_converter = false;
    int num_instances = 1;
    int http_threads = 4;
    int process_timeout = 120;
    size_t max_upload_size = 25 * 1024 * 1024; // 25 MB
    int read_timeout = 600;    // seconds
    int write_timeout = 600;   // seconds
};

struct params {
    int32_t n_threads     = std::min(4, (int32_t) std::thread::hardware_concurrency());
    int32_t n_processors  = 1;
    int32_t offset_t_ms   = 0;
    int32_t offset_n      = 0;
    int32_t duration_ms   = 0;
    int32_t progress_step = 5;
    int32_t max_context   = -1;
    int32_t max_len       = 0;
    int32_t best_of       = 2;
    int32_t beam_size     = -1;
    int32_t audio_ctx     = 0;

    float word_thold      =  0.01f;
    float entropy_thold   =  2.40f;
    float logprob_thold   = -1.00f;
    float temperature     =  0.00f;
    float temperature_inc =  0.20f;

    bool debug_mode      = false;
    bool translate       = false;
    bool detect_language = false;
    bool diarize         = false;
    bool tinydiarize     = false;
    bool split_on_word   = false;
    bool no_fallback     = false;
    bool print_special   = false;
    bool print_colors    = false;
    bool print_realtime  = false;
    bool print_progress  = false;
    bool no_timestamps   = false;
    bool use_gpu         = true;
    bool flash_attn      = false;

    std::string language        = "en";
    std::string prompt          = "";
    std::string font_path       = "/System/Library/Fonts/Supplemental/Courier New Bold.ttf";
    std::string model           = "models/ggml-base.en.bin";

    std::string response_format = json_format;

    // [TDRZ] speaker turn string
    std::string tdrz_speaker_turn = " [SPEAKER_TURN]"; // TODO: set from command line

    std::string openvino_encode_device = "CPU";

    std::string dtw = "";
};

} // namespace whisper