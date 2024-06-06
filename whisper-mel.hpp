#pragma once
#include "ggml-backend.h"
#include <vector>

struct whisper_mel {
    int n_len_org = 0;

    ggml_tensor * tensor = nullptr;
    ggml_context * ctx = nullptr;
    ggml_backend_buffer_t buffer = nullptr;

    whisper_mel() = default;
    ~whisper_mel();

    whisper_mel(const whisper_mel &) = delete;
    whisper_mel & operator=(const whisper_mel &) = delete;
    whisper_mel(whisper_mel &&) noexcept;
    whisper_mel & operator=(whisper_mel &&) noexcept;

    void init(ggml_backend_t backend, int n_len, int n_len_org, int n_mel);
    void reset();
    void take(whisper_mel & other) noexcept;
};

struct whisper_filters {
    int32_t n_mel;
    int32_t n_fft;

    std::vector<float> data;
};

template <typename T>
struct whisper_span {
    T * data;
    int len;
};

struct whisper_mel_calc {
    virtual ~whisper_mel_calc();
    virtual whisper_mel calculate(whisper_span<const float> samples, int n_threads) const = 0;
    static whisper_span<const float> hann_window();
};

// returns a new pointer which needs to be freed with delete
whisper_mel_calc * whisper_mel_calc_create(ggml_backend_t backend, const whisper_filters & filters);
