#ifndef WHISPER_H
#define WHISPER_H

#include <stdint.h>

#ifdef WHISPER_SHARED
#    ifdef _WIN32
#        ifdef WHISPER_BUILD
#            define WHISPER_API __declspec(dllexport)
#        else
#            define WHISPER_API __declspec(dllimport)
#        endif
#    else
#        define WHISPER_API __attribute__ ((visibility ("default")))
#    endif
#else
#    define WHISPER_API
#endif

#define WHISPER_SAMPLE_RATE 16000
#define WHISPER_N_FFT       400
#define WHISPER_N_MEL       80
#define WHISPER_HOP_LENGTH  160
#define WHISPER_CHUNK_SIZE  30

#ifdef __cplusplus
extern "C" {
#endif

    //
    // C interface
    //

    // TODO: documentation will come soon

    struct whisper_context;

    typedef int whisper_token;

    WHISPER_API struct whisper_context * whisper_init(const char * path_model);
    WHISPER_API void whisper_free(struct whisper_context * ctx);

    WHISPER_API int whisper_pcm_to_mel(
            struct whisper_context * ctx,
            const float * samples,
            int n_samples,
            int n_threads);

    // n_mel must be 80
    WHISPER_API int whisper_set_mel(
            struct whisper_context * ctx,
            const float * data,
            int n_len,
            int n_mel);

    WHISPER_API int whisper_encode(
            struct whisper_context * ctx,
            int offset,
            int n_threads);

    WHISPER_API int whisper_decode(
            struct whisper_context * ctx,
            const whisper_token * tokens,
            int n_tokens,
            int n_past,
            int n_threads);

    WHISPER_API whisper_token whisper_sample_best(struct whisper_context * ctx, bool need_timestamp);
    WHISPER_API whisper_token whisper_sample_timestamp(struct whisper_context * ctx);

    // return the id of the specified language, returns -1 if not found
    WHISPER_API int whisper_lang_id(const char * lang);

    WHISPER_API int     whisper_n_len          (struct whisper_context * ctx); // mel length
    WHISPER_API int     whisper_n_vocab        (struct whisper_context * ctx);
    WHISPER_API int     whisper_n_text_ctx     (struct whisper_context * ctx);
    WHISPER_API int     whisper_is_multilingual(struct whisper_context * ctx);
    WHISPER_API float * whisper_get_probs      (struct whisper_context * ctx);

    WHISPER_API const char * whisper_token_to_str(struct whisper_context * ctx, whisper_token token);

    WHISPER_API whisper_token whisper_token_eot (struct whisper_context * ctx);
    WHISPER_API whisper_token whisper_token_sot (struct whisper_context * ctx);
    WHISPER_API whisper_token whisper_token_prev(struct whisper_context * ctx);
    WHISPER_API whisper_token whisper_token_solm(struct whisper_context * ctx);
    WHISPER_API whisper_token whisper_token_not (struct whisper_context * ctx);
    WHISPER_API whisper_token whisper_token_beg (struct whisper_context * ctx);

    WHISPER_API whisper_token whisper_token_translate ();
    WHISPER_API whisper_token whisper_token_transcribe();

    WHISPER_API void whisper_print_timings(struct whisper_context * ctx);

    ////////////////////////////////////////////////////////////////////////////

    enum whisper_decode_strategy {
        WHISPER_DECODE_GREEDY,
        WHISPER_DECODE_BEAM_SEARCH,
    };

    struct whisper_full_params {
        enum whisper_decode_strategy strategy;

        int n_threads;

        bool translate;
        bool print_special_tokens;
        bool print_progress;
        bool print_realtime;
        bool print_timestamps;

        const char * language;

        union {
            struct {
                int n_past;
            } greedy;

            struct {
                int n_past;
                int beam_width;
                int n_best;
            } beam_search;
        };
    };

    WHISPER_API struct whisper_full_params whisper_full_default_params(enum whisper_decode_strategy strategy);

    // full whisper run - encode + decode
    WHISPER_API int whisper_full(
            struct whisper_context * ctx,
            struct whisper_full_params params,
            const float * samples,
            int n_samples);

    WHISPER_API int whisper_full_n_segments(struct whisper_context * ctx);

    WHISPER_API int64_t whisper_full_get_segment_t0(struct whisper_context * ctx, int i_segment);
    WHISPER_API int64_t whisper_full_get_segment_t1(struct whisper_context * ctx, int i_segment);

    WHISPER_API const char * whisper_full_get_segment_text(struct whisper_context * ctx, int i_segment);

#ifdef __cplusplus
}
#endif

#endif
