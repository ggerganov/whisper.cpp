#include "stream_components_server.h"

using namespace stream_components;


// -- WhisperServer -- 

WhisperServer::WhisperServer(
    const struct server_params & server_params,
    const struct audio_params & audio_params):
        server_params(server_params),
        audio_params(audio_params),
        ctx(whisper_init_from_file(server_params.model.c_str()))
{
    {
        fprintf(stderr, "\n");
        if (!whisper_is_multilingual(ctx)) {
            if (server_params.language != "en" || server_params.translate) {
                this->server_params.language = "en";
                this->server_params.translate = false;
                fprintf(stderr, "%s: WARNING: model is not multilingual, ignoring language and translation options\n", __func__);
            }
        }
        fprintf(stderr, "%s: serving with %d threads, lang = %s, task = %s, timestamps = %d ...\n",
            __func__,
            server_params.n_threads,
            server_params.language.c_str(),
            server_params.translate ? "translate" : "transcribe",
            server_params.no_timestamps ? 0 : 1);

        // if (!params.use_vad) {
        //     fprintf(stderr, "%s: n_new_line = %d, no_context = %d\n", __func__, n_new_line, params.no_context);
        // } 

        fprintf(stderr, "\n");
    }
}

WhisperServer::~WhisperServer()
{
    whisper_print_timings(ctx);
    whisper_free(ctx);
}

WhisperOutputPtr WhisperServer::process(const float * samples, int sample_count) 
{
    whisper_full_params wparams = whisper_full_default_params(WHISPER_SAMPLING_GREEDY);

    wparams.print_progress   = false;
    wparams.print_realtime   = false;
    wparams.print_timestamps = false;
    wparams.print_special    = true;
    wparams.max_tokens       = 0;
    wparams.token_timestamps = true;

    wparams.translate        = server_params.translate;
    wparams.single_segment   = !audio_params.use_vad;
    wparams.language         = server_params.language.c_str();
    wparams.n_threads        = server_params.n_threads;

    wparams.audio_ctx        = audio_params.audio_ctx;
    wparams.speed_up         = server_params.speed_up;

    wparams.tdrz_enable      = server_params.tinydiarize; // [TDRZ]

    // disable temperature fallback
    //wparams.temperature_inc  = -1.0f;
    wparams.temperature_inc  = server_params.no_fallback ? 0.0f : wparams.temperature_inc;

    wparams.prompt_tokens    = server_params.no_context ? nullptr : prompt_tokens.data();
    wparams.prompt_n_tokens  = server_params.no_context ? 0       : prompt_tokens.size();
    
    // *** Run the actual inference!!! ***
    if (whisper_full(ctx, wparams, samples, sample_count) != 0) {
        return nullptr;
    }

    // Now sure whether n_iter and n_new_line should have ever been there...
    // *** SUSPICIOUS what happens by removing them? Are they essential?
    //if (!use_vad && (n_iter % n_new_line) == 0) {
    if (!audio_params.use_vad) {
        //printf("\n");

        // keep part of the audio for next iteration to try to mitigate word boundary issues
        // *** I don't know if we need this...
        //pcmf32_old = std::vector<float>(pcmf32.end() - n_samples_keep, pcmf32.end());

        // Add tokens of the last full length segment as the prompt
        if (!server_params.no_context) {
            prompt_tokens.clear();

            const int n_segments = whisper_full_n_segments(ctx);
            for (int i = 0; i < n_segments; ++i) {
                const int token_count = whisper_full_n_tokens(ctx, i);
                for (int j = 0; j < token_count; ++j) {
                    prompt_tokens.push_back(whisper_full_get_token_id(ctx, i, j));
                }
            }
        }
    }

    auto r = std::make_shared<WhisperOutput>(ctx, server_params);

    return r;
}
