#ifndef WHISPER_STREAM_COMPONENTS_AUDIO_H
#define WHISPER_STREAM_COMPONENTS_AUDIO_H

#include "common-sdl.h"
#include "common.h"
#include "whisper.h"
#include "stream_components_params.h"

namespace stream_components {

/**
 * Encapsulates audio capture and processing.
 * Represents an SDL audio device
 */ 
class LocalSDLMicrophone {
public:
    LocalSDLMicrophone(audio_params & params);
    ~LocalSDLMicrophone();

    std::vector<float>& get_next();

protected:
    audio_params params;
    
    audio_async audio;
    std::chrono::steady_clock::time_point t_last;
    std::chrono::steady_clock::time_point t_start;

    std::vector<float> pcmf32_new;
    std::vector<float> pcmf32_old;
    std::vector<float> pcmf32;
};

} // namespace stream_components

#endif // WHISPER_STREAM_COMPONENTS_AUDIO_H
