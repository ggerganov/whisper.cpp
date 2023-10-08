#ifndef WHISPER_STREAM_COMPONENTS_PARAMS_H
#define WHISPER_STREAM_COMPONENTS_PARAMS_H

#include <string>
#include <thread>

namespace stream_components {

struct audio_params {
    int32_t step_ms    = 3000;
    int32_t length_ms  = 10000;
    int32_t keep_ms    = 200;

    int32_t capture_id = -1;
    int32_t audio_ctx  = 0;

    int32_t n_samples_step = 0;
    int32_t n_samples_keep = 0;
    int32_t n_samples_len = 0;
    int32_t n_samples_30s = 0;
    bool use_vad = false;

    float vad_thold     = 0.6f;
    float freq_thold    = 100.0f;

    void initialize() {
        keep_ms   = std::min(keep_ms,   step_ms);
        length_ms = std::max(length_ms, step_ms);

        n_samples_step = int32_t(1e-3*step_ms*WHISPER_SAMPLE_RATE);
        n_samples_keep = int32_t(1e-3*keep_ms*WHISPER_SAMPLE_RATE);
        n_samples_len  = int32_t(1e-3*length_ms*WHISPER_SAMPLE_RATE);
        n_samples_30s  = int32_t(1e-3*30000.0*WHISPER_SAMPLE_RATE);
        use_vad        = n_samples_step <= 0;
    }
};

struct server_params {
    int32_t n_threads  = std::min(4, (int32_t) std::thread::hardware_concurrency());
    bool speed_up      = false;
    bool translate     = false;
    bool no_fallback   = false;
    bool no_context    = true;
    bool no_timestamps = false;
    bool save_audio    = false;
    
    bool tinydiarize   = false;
    bool diarize       = false;

    std::string language  = "en";
    std::string model     = "models/ggml-base.en.bin";

    void initialize() {}
};

} // namespace stream_components

#endif // WHISPER_STREAM_COMPONENTS_PARAMS_H
