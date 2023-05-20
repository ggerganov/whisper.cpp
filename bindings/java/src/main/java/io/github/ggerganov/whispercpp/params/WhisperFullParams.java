package io.github.ggerganov.whispercpp.params;

import com.sun.jna.Callback;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;
import io.github.ggerganov.whispercpp.callbacks.WhisperEncoderBeginCallback;
import io.github.ggerganov.whispercpp.callbacks.WhisperLogitsFilterCallback;
import io.github.ggerganov.whispercpp.callbacks.WhisperNewSegmentCallback;
import io.github.ggerganov.whispercpp.callbacks.WhisperProgressCallback;

/**
 * Parameters for the whisper_full() function.
 * If you change the order or add new parameters, make sure to update the default values in whisper.cpp:
 * whisper_full_default_params()
 */
public class WhisperFullParams extends Structure {

    /** Sampling strategy for whisper_full() function. */
    public int strategy;

    /** Number of threads. */
    public int n_threads;

    /** Maximum tokens to use from past text as a prompt for the decoder. */
    public int n_max_text_ctx;

    /** Start offset in milliseconds. */
    public int offset_ms;

    /** Audio duration to process in milliseconds. */
    public int duration_ms;

    /** Translate flag. */
    public boolean translate;

    /** Flag to indicate whether to use past transcription (if any) as an initial prompt for the decoder. */
    public boolean no_context;

    /** Flag to force single segment output (useful for streaming). */
    public boolean single_segment;

    /** Flag to print special tokens (e.g., &lt;SOT>, &lt;EOT>, &lt;BEG>, etc.). */
    public boolean print_special;

    /** Flag to print progress information. */
    public boolean print_progress;

    /** Flag to print results from within whisper.cpp (avoid it, use callback instead). */
    public boolean print_realtime;

    /** Flag to print timestamps for each text segment when printing realtime. */
    public boolean print_timestamps;

    /** [EXPERIMENTAL] Flag to enable token-level timestamps. */
    public boolean token_timestamps;

    /** [EXPERIMENTAL] Timestamp token probability threshold (~0.01). */
    public float thold_pt;

    /** [EXPERIMENTAL] Timestamp token sum probability threshold (~0.01). */
    public float thold_ptsum;

    /** Maximum segment length in characters. */
    public int max_len;

    /** Flag to split on word rather than on token (when used with max_len). */
    public boolean split_on_word;

    /** Maximum tokens per segment (0 = no limit). */
    public int max_tokens;

    /** Flag to speed up the audio by 2x using Phase Vocoder. */
    public boolean speed_up;

    /** Overwrite the audio context size (0 = use default). */
    public int audio_ctx;

    /** Tokens to provide to the whisper decoder as an initial prompt.
     * These are prepended to any existing text context from a previous call. */
    public String initial_prompt;

    /** Prompt tokens. */
    public Pointer prompt_tokens;

    /** Number of prompt tokens. */
    public int prompt_n_tokens;

    /** Language for auto-detection.
     * For auto-detection, set to `null`, `""`, or "auto". */
    public String language;

    /** Flag to indicate whether to detect language automatically. */
    public boolean detect_language;

    /** Common decoding parameters. */

    /** Flag to suppress blank tokens. */
    public boolean suppress_blank;

    /** Flag to suppress non-speech tokens. */
    public boolean suppress_non_speech_tokens;

    /** Initial decoding temperature. */
    public float temperature;

    /** Maximum initial timestamp. */
    public float max_initial_ts;

    /** Length penalty. */
    public float length_penalty;

    /** Fallback parameters. */

    /** Temperature increment. */
    public float temperature_inc;

    /** Entropy threshold (similar to OpenAI's "compression_ratio_threshold"). */
    public float entropy_thold;

    /** Log probability threshold. */
    public float logprob_thold;

    /** No speech threshold. */
    public float no_speech_thold;

    class GreedyParams extends Structure {
        /** https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L264 */
        public int best_of;
    }

    /** Greedy decoding parameters. */
    public GreedyParams greedy;

    class BeamSearchParams extends Structure {
        /** ref: https://github.com/openai/whisper/blob/f82bc59f5ea234d4b97fb2860842ed38519f7e65/whisper/transcribe.py#L265 */
        int beam_size;

        /** ref: https://arxiv.org/pdf/2204.05424.pdf */
        float patience;
    }

    /**
     * Beam search decoding parameters.
     */
    public BeamSearchParams beam_search;

    /**
     * Callback for every newly generated text segment.
     */
    public WhisperNewSegmentCallback new_segment_callback;

    /**
     * User data for the new_segment_callback.
     */
    public Pointer new_segment_callback_user_data;

    /**
     * Callback on each progress update.
     */
    public WhisperProgressCallback progress_callback;

    /**
     * User data for the progress_callback.
     */
    public Pointer progress_callback_user_data;

    /**
     * Callback each time before the encoder starts.
     */
    public WhisperEncoderBeginCallback encoder_begin_callback;

    /**
     * User data for the encoder_begin_callback.
     */
    public Pointer encoder_begin_callback_user_data;

    /**
     * Callback by each decoder to filter obtained logits.
     */
    public WhisperLogitsFilterCallback logits_filter_callback;

    /**
     * User data for the logits_filter_callback.
     */
    public Pointer logits_filter_callback_user_data;
}

