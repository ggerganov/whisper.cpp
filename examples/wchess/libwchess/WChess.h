#pragma once
#include "whisper.h"
#include <string>
#include <vector>
#include <memory>

class Chessboard;

class WChess {
public:
    using SetStatusCb = void (*)(const std::string &);
    using CheckRunningCb = bool (*)();
    using GetAudioCb = void (*)(int, std::vector<float> &);
    using SetMovesCb = void (*)(const std::string &);
    using ClearAudioCb = void (*)();

    struct callbacks {
        SetStatusCb set_status = nullptr;
        CheckRunningCb check_running = nullptr;
        GetAudioCb get_audio = nullptr;
        SetMovesCb set_moves = nullptr;
        ClearAudioCb clear_audio = nullptr;
    };

    struct settings {
        int32_t vad_ms     = 2000;
        int32_t prompt_ms  = 5000;
        int32_t command_ms = 4000;
        float vad_thold    = 0.2f;
        float freq_thold   = 100.0f;
        bool print_energy  = false;
    };

    WChess(
        whisper_context * ctx,
        const whisper_full_params & wparams,
        callbacks cb,
        settings s
    );
    ~WChess();

    void run();
    std::string stringify_board() const;
private:
    void get_audio(int ms, std::vector<float>& pcmf32) const;
    void set_status(const std::string& msg) const;
    void set_moves(const std::string& moves) const;
    bool check_running() const;
    void clear_audio() const;
    std::string transcribe(
                    const std::vector<float> & pcmf32,
                    float & logprob_min,
                    float & logprob_sum,
                    int & n_tokens,
                    int64_t & t_ms);

    whisper_context * m_ctx;
    whisper_full_params m_wparams;
    const callbacks m_cb;
    const settings m_settings;
    std::unique_ptr<Chessboard> m_board;
};
