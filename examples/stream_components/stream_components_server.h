#ifndef WHISPER_STREAM_COMPONENTS_SERVER_H
#define WHISPER_STREAM_COMPONENTS_SERVER_H

#include "whisper.h"
#include "stream_components_params.h"
#include "stream_components_output.h"

using std::shared_ptr;

namespace stream_components {

class WhisperServer {
public:
    WhisperServer(
        const struct server_params & server_params,
        const struct audio_params & audio_params);
    ~WhisperServer();

    WhisperOutputPtr process(
        const float * samples, 
        int size);

    server_params server_params;
    audio_params audio_params;

    struct whisper_context * ctx;
    std::vector<whisper_token> prompt_tokens;
};

} // namespace stream_components

#endif // WHISPER_STREAM_COMPONENTS_SERVER_H
