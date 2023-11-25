#pragma once
#include "Chessboard.h"
#include "whisper.h"
#include <string>
#include <vector>

class Chess {
public:
    using StatusSetter = void (*)(const std::string & status);
    using ISRunning = bool (*)();
    using AudioGetter = void (*)(int, std::vector<float>&);
    using MovesSetter = void (*)(const std::string & moves);
    Chess(  whisper_context * ctx,
            const whisper_full_params & wparams,
            StatusSetter status_setter,
            ISRunning running,
            AudioGetter audio,
            MovesSetter moveSetter);
    void run();
    std::string stringifyBoard();
private:
    void get_audio(int ms, std::vector<float>& pcmf32);
    void set_status(const char* msg);
    void set_moves(const std::string& moves);
    bool check_running();
    std::string transcribe(
                    const std::vector<float> & pcmf32,
                    float & logprob_min,
                    float & logprob_sum,
                    int & n_tokens,
                    int64_t & t_ms);
    whisper_context * m_ctx;
    whisper_full_params m_wparams;
    StatusSetter m_status_setter;
    ISRunning m_running;
    AudioGetter m_audio;
    MovesSetter m_moveSetter;
    Chessboard m_board;
};
