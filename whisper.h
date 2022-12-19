#ifndef WHISPER_H
#define WHISPER_H

#include <stdint.h>
#include <stdbool.h>

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
    // The following interface is thread-safe as long as the sample whisper_context is not used by multiple threads
    // concurrently.
    //
    // Basic usage:
    //
    //     #include "whisper.h"
    //
    //     ...
    //
    //     struct whisper_context * ctx = whisper_init("/path/to/ggml-base.en.bin");
    //
    //     if (whisper_full(ctx, wparams, pcmf32.data(), pcmf32.size()) != 0) {
    //         fprintf(stderr, "failed to process audio\n");
    //         return 7;
    //     }
    //
    //     const int n_segments = whisper_full_n_segments(ctx);
    //     for (int i = 0; i < n_segments; ++i) {
    //         const char * text = whisper_full_get_segment_text(ctx, i);
    //         printf("%s", text);
    //     }
    //
    //     whisper_free(ctx);
    //
    //     ...
    //
    // This is a demonstration of the most straightforward usage of the library.
    // "pcmf32" contains the RAW audio data in 32-bit floating point format.
    //
    // The interface also allows for more fine-grained control over the computation, but it requires a deeper
    // understanding of how the model works.
    //

    struct whisper_context;

    typedef int whisper_token;

    typedef struct whisper_token_data {
        whisper_token id;  // token id
        whisper_token tid; // forced timestamp token id

        float p;           // probability of the token
        float pt;          // probability of the timestamp token
        float ptsum;       // sum of probabilities of all timestamp tokens

        // token-level timestamp data
        // do not use if you haven't computed token-level timestamps
        int64_t t0;        // start time of the token
        int64_t t1;        //   end time of the token

        float vlen;        // voice length of the token
    } whisper_token_data;

    // Allocates all memory needed for the model and loads the model from the given file.
    // Returns NULL on failure.
    WHISPER_API struct whisper_context * whisper_init(const char * path_model);

    // Frees all memory allocated by the model.
    WHISPER_API void whisper_free(struct whisper_context * ctx);

    // Convert RAW PCM audio to log mel spectrogram.
    // The resulting spectrogram is stored inside the provided whisper context.
    // Returns 0 on success
    WHISPER_API int whisper_pcm_to_mel(
            struct whisper_context * ctx,
                       const float * samples,
                               int   n_samples,
                               int   n_threads);

    // This can be used to set a custom log mel spectrogram inside the provided whisper context.
    // Use this instead of whisper_pcm_to_mel() if you want to provide your own log mel spectrogram.
    // n_mel must be 80
    // Returns 0 on success
    WHISPER_API int whisper_set_mel(
            struct whisper_context * ctx,
                       const float * data,
                               int   n_len,
                               int   n_mel);

    // Run the Whisper encoder on the log mel spectrogram stored inside the provided whisper context.
    // Make sure to call whisper_pcm_to_mel() or whisper_set_mel() first.
    // offset can be used to specify the offset of the first frame in the spectrogram.
    // Returns 0 on success
    WHISPER_API int whisper_encode(
            struct whisper_context * ctx,
                               int   offset,
                               int   n_threads);

    // Run the Whisper decoder to obtain the logits and probabilities for the next token.
    // Make sure to call whisper_encode() first.
    // tokens + n_tokens is the provided context for the decoder.
    // n_past is the number of tokens to use from previous decoder calls.
    // Returns 0 on success
    WHISPER_API int whisper_decode(
            struct whisper_context * ctx,
               const whisper_token * tokens,
                               int   n_tokens,
                               int   n_past,
                               int   n_threads);

    // Token sampling methods.
    // These are provided for convenience and can be used after each call to whisper_decode().
    // You can also implement your own sampling method using the whisper_get_probs() function.
    // whisper_sample_best() returns the token with the highest probability
    // whisper_sample_timestamp() returns the most probable timestamp token
    WHISPER_API whisper_token_data whisper_sample_best(struct whisper_context * ctx);
    WHISPER_API whisper_token_data whisper_sample_timestamp(struct whisper_context * ctx, bool is_initial);

    // Convert the provided text into tokens.
    // The tokens pointer must be large enough to hold the resulting tokens.
    // Returns the number of tokens on success, no more than n_max_tokens
    // Returns -1 on failure
    // TODO: not sure if correct
    WHISPER_API int whisper_tokenize(
            struct whisper_context * ctx,
                        const char * text,
                     whisper_token * tokens,
	                           int   n_max_tokens);

    // Largest language id (i.e. number of available languages - 1)
    WHISPER_API int whisper_lang_max_id();

    // Return the id of the specified language, returns -1 if not found
    // Examples:
    //   "de" -> 2
    //   "german" -> 2
    WHISPER_API int whisper_lang_id(const char * lang);

    // Return the short string of the specified language id (e.g. 2 -> "de"), returns nullptr if not found
    WHISPER_API const char * whisper_lang_str(int id);

    // Use mel data at offset_ms to try and auto-detect the spoken language
    // Make sure to call whisper_pcm_to_mel() or whisper_set_mel() first
    // Returns the top language id or negative on failure
    // If not null, fills the lang_probs array with the probabilities of all languages
    // The array must be whispe_lang_max_id() + 1 in size
    // ref: https://github.com/openai/whisper/blob/main/whisper/decoding.py#L18-L69
    WHISPER_API int whisper_lang_auto_detect(
            struct whisper_context * ctx,
                               int   offset_ms,
                               int   n_threads,
                             float * lang_probs);

    WHISPER_API int whisper_n_len          (struct whisper_context * ctx); // mel length
    WHISPER_API int whisper_n_vocab        (struct whisper_context * ctx);
    WHISPER_API int whisper_n_text_ctx     (struct whisper_context * ctx);
    WHISPER_API int whisper_is_multilingual(struct whisper_context * ctx);

    // The probabilities for the next token
    WHISPER_API float * whisper_get_probs(struct whisper_context * ctx);

    // Token Id -> String. Uses the vocabulary in the provided context
    WHISPER_API const char * whisper_token_to_str(struct whisper_context * ctx, whisper_token token);

    // Special tokens
    WHISPER_API whisper_token whisper_token_eot (struct whisper_context * ctx);
    WHISPER_API whisper_token whisper_token_sot (struct whisper_context * ctx);
    WHISPER_API whisper_token whisper_token_prev(struct whisper_context * ctx);
    WHISPER_API whisper_token whisper_token_solm(struct whisper_context * ctx);
    WHISPER_API whisper_token whisper_token_not (struct whisper_context * ctx);
    WHISPER_API whisper_token whisper_token_beg (struct whisper_context * ctx);
    WHISPER_API whisper_token whisper_token_lang(struct whisper_context * ctx, int lang_id);

    // Task tokens
    WHISPER_API whisper_token whisper_token_translate (void);
    WHISPER_API whisper_token whisper_token_transcribe(void);

    // Performance information
    WHISPER_API void whisper_print_timings(struct whisper_context * ctx);
    WHISPER_API void whisper_reset_timings(struct whisper_context * ctx);

    // Print system information
    WHISPER_API const char * whisper_print_system_info(void);

    ////////////////////////////////////////////////////////////////////////////

    // Available sampling strategies
    enum whisper_sampling_strategy {
        WHISPER_SAMPLING_GREEDY,      // Always select the most probable token
        WHISPER_SAMPLING_BEAM_SEARCH, // TODO: not implemented yet!
    };

    // Text segment callback
    // Called on every newly generated text segment
    // Use the whisper_full_...() functions to obtain the text segments
    typedef void (*whisper_new_segment_callback)(struct whisper_context * ctx, int n_new, void * user_data);

    // Encoder begin callback
    // If not NULL, called before the encoder starts
    // If it returns false, the computation is aborted
    typedef bool (*whisper_encoder_begin_callback)(struct whisper_context * ctx, void * user_data);

    // Parameters for the whisper_full() function
    // If you chnage the order or add new parameters, make sure to update the default values in whisper.cpp:
    // whisper_full_default_params()
    struct whisper_full_params {
        enum whisper_sampling_strategy strategy;

        int n_threads;
        int n_max_text_ctx;
        int offset_ms;          // start offset in ms
        int duration_ms;        // audio duration to process in ms

        bool translate;
        bool no_context;
        bool single_segment;    // force single segment output (useful for streaming)
        bool print_special;
        bool print_progress;
        bool print_realtime;
        bool print_timestamps;

        // [EXPERIMENTAL] token-level timestamps
        bool  token_timestamps; // enable token-level timestamps
        float thold_pt;         // timestamp token probability threshold (~0.01)
        float thold_ptsum;      // timestamp token sum probability threshold (~0.01)
        int   max_len;          // max segment length in characters
        int   max_tokens;       // max tokens per segment (0 = no limit)

        // [EXPERIMENTAL] speed-up techniques
        bool speed_up;          // speed-up the audio by 2x using Phase Vocoder
        int  audio_ctx;         // overwrite the audio context size (0 = use default)

        // tokens to provide the whisper model as initial prompt
        // these are prepended to any existing text context from a previous call
        const whisper_token * prompt_tokens;
        int prompt_n_tokens;

        // for auto-detection, set to nullptr, "" or "auto"
        const char * language;

        struct {
            int n_past;
        } greedy;

        struct {
            int n_past;
            int beam_width;
            int n_best;
        } beam_search;

        whisper_new_segment_callback new_segment_callback;
        void * new_segment_callback_user_data;

        whisper_encoder_begin_callback encoder_begin_callback;
        void * encoder_begin_callback_user_data;
    };

    WHISPER_API struct whisper_full_params whisper_full_default_params(enum whisper_sampling_strategy strategy);

    // Run the entire model: PCM -> log mel spectrogram -> encoder -> decoder -> text
    // Uses the specified decoding strategy to obtain the text.
    WHISPER_API int whisper_full(
                struct whisper_context * ctx,
            struct whisper_full_params   params,
                           const float * samples,
                                   int   n_samples);

    // Split the input audio in chunks and process each chunk separately using whisper_full()
    // It seems this approach can offer some speedup in some cases.
    // However, the transcription accuracy can be worse at the beginning and end of each chunk.
    WHISPER_API int whisper_full_parallel(
                struct whisper_context * ctx,
            struct whisper_full_params   params,
                           const float * samples,
                                   int   n_samples,
                                   int   n_processors);

    // Number of generated text segments.
    // A segment can be a few words, a sentence, or even a paragraph.
    WHISPER_API int whisper_full_n_segments(struct whisper_context * ctx);

    // Get the start and end time of the specified segment.
    WHISPER_API int64_t whisper_full_get_segment_t0(struct whisper_context * ctx, int i_segment);
    WHISPER_API int64_t whisper_full_get_segment_t1(struct whisper_context * ctx, int i_segment);

    // Get the text of the specified segment.
    WHISPER_API const char * whisper_full_get_segment_text(struct whisper_context * ctx, int i_segment);

    // Get number of tokens in the specified segment.
    WHISPER_API int whisper_full_n_tokens(struct whisper_context * ctx, int i_segment);

    // Get the token text of the specified token in the specified segment.
    WHISPER_API const char * whisper_full_get_token_text(struct whisper_context * ctx, int i_segment, int i_token);
    WHISPER_API whisper_token whisper_full_get_token_id (struct whisper_context * ctx, int i_segment, int i_token);

    // Get token data for the specified token in the specified segment.
    // This contains probabilities, timestamps, etc.
    WHISPER_API whisper_token_data whisper_full_get_token_data(struct whisper_context * ctx, int i_segment, int i_token);

    // Get the probability of the specified token in the specified segment.
    WHISPER_API float whisper_full_get_token_p(struct whisper_context * ctx, int i_segment, int i_token);

#ifdef __cplusplus
}
#endif

#endif
