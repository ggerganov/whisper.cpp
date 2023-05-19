package io.github.ggerganov.whispercpp;

import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.Platform;
import com.sun.jna.Pointer;
import io.github.ggerganov.whispercpp.model.WhisperModelLoader;
import io.github.ggerganov.whispercpp.model.WhisperTokenData;

public interface WhisperJnaLibrary extends Library {
    WhisperJnaLibrary instance = Native.load(Platform.isWindows()
            ? "whispercpp.dll"
            : "whispercpp",
            WhisperJnaLibrary.class);

    String whisper_print_system_info();


    /**
     * Benchmark function for memcpy.
     *
     * @param nThreads Number of threads to use for the benchmark.
     * @return The result of the benchmark.
     */
    int whisper_bench_memcpy(int nThreads);

    /**
     * Benchmark function for memcpy as a string.
     *
     * @param nThreads Number of threads to use for the benchmark.
     * @return The result of the benchmark as a string.
     */
    String whisper_bench_memcpy_str(int nThreads);

    /**
     * Benchmark function for ggml_mul_mat.
     *
     * @param nThreads Number of threads to use for the benchmark.
     * @return The result of the benchmark.
     */
    int whisper_bench_ggml_mul_mat(int nThreads);

    /**
     * Benchmark function for ggml_mul_mat as a string.
     *
     * @param nThreads Number of threads to use for the benchmark.
     * @return The result of the benchmark as a string.
     */
    String whisper_bench_ggml_mul_mat_str(int nThreads);

    /**
     * Allocate (almost) all memory needed for the model by loading from a file.
     *
     * @param path_model Path to the model file
     * @return Whisper context on success, null on failure
     */
    Pointer whisper_init_from_file(String path_model);

    /**
     * Allocate (almost) all memory needed for the model by loading from a buffer.
     *
     * @param buffer       Model buffer
     * @param buffer_size  Size of the model buffer
     * @return Whisper context on success, null on failure
     */
    Pointer whisper_init_from_buffer(Pointer buffer, int buffer_size);

    /**
     * Allocate (almost) all memory needed for the model using a model loader.
     *
     * @param loader Model loader
     * @return Whisper context on success, null on failure
     */
    Pointer whisper_init(WhisperModelLoader loader);

    /**
     * Allocate (almost) all memory needed for the model by loading from a file without allocating the state.
     *
     * @param path_model Path to the model file
     * @return Whisper context on success, null on failure
     */
    Pointer whisper_init_from_file_no_state(String path_model);

    /**
     * Allocate (almost) all memory needed for the model by loading from a buffer without allocating the state.
     *
     * @param buffer       Model buffer
     * @param buffer_size  Size of the model buffer
     * @return Whisper context on success, null on failure
     */
    Pointer whisper_init_from_buffer_no_state(Pointer buffer, int buffer_size);

//    Pointer whisper_init_from_buffer_no_state(Pointer buffer, long buffer_size);

    /**
     * Allocate (almost) all memory needed for the model using a model loader without allocating the state.
     *
     * @param loader Model loader
     * @return Whisper context on success, null on failure
     */
    Pointer whisper_init_no_state(WhisperModelLoader loader);

    /**
     * Allocate memory for the Whisper state.
     *
     * @param ctx Whisper context
     * @return Whisper state on success, null on failure
     */
    Pointer whisper_init_state(Pointer ctx);

    /**
     * Free all allocated memory associated with the Whisper context.
     *
     * @param ctx Whisper context
     */
    void whisper_free(Pointer ctx);

    /**
     * Free all allocated memory associated with the Whisper state.
     *
     * @param state Whisper state
     */
    void whisper_free_state(Pointer state);


    /** Get the start time of the specified segment. */
    long whisper_full_get_segment_t0(Pointer ctx, int i_segment);

    /** Get the start time of the specified segment from the state. */
    long whisper_full_get_segment_t0_from_state(Pointer state, int i_segment);

    /** Get the end time of the specified segment. */
    long whisper_full_get_segment_t1(Pointer ctx, int i_segment);

    /** Get the end time of the specified segment from the state. */
    long whisper_full_get_segment_t1_from_state(Pointer state, int i_segment);

    /** Get the text of the specified segment. */
    String whisper_full_get_segment_text(Pointer ctx, int i_segment);

    /** Get the text of the specified segment from the state. */
    String whisper_full_get_segment_text_from_state(Pointer state, int i_segment);

    /** Get the number of tokens in the specified segment. */
    int whisper_full_n_tokens(Pointer ctx, int i_segment);

    /** Get the number of tokens in the specified segment from the state. */
    int whisper_full_n_tokens_from_state(Pointer state, int i_segment);

    /** Get the token text of the specified token in the specified segment. */
    String whisper_full_get_token_text(Pointer ctx, int i_segment, int i_token);


    /** Get the token text of the specified token in the specified segment from the state. */
    String whisper_full_get_token_text_from_state(Pointer ctx, Pointer state, int i_segment, int i_token);

    /** Get the token ID of the specified token in the specified segment. */
    int whisper_full_get_token_id(Pointer ctx, int i_segment, int i_token);

    /** Get the token ID of the specified token in the specified segment from the state. */
    int whisper_full_get_token_id_from_state(Pointer state, int i_segment, int i_token);

    /** Get token data for the specified token in the specified segment. */
    WhisperTokenData whisper_full_get_token_data(Pointer ctx, int i_segment, int i_token);

    /** Get token data for the specified token in the specified segment from the state. */
    WhisperTokenData whisper_full_get_token_data_from_state(Pointer state, int i_segment, int i_token);

    /** Get the probability of the specified token in the specified segment. */
    float whisper_full_get_token_p(Pointer ctx, int i_segment, int i_token);

    /** Get the probability of the specified token in the specified segment from the state. */
    float whisper_full_get_token_p_from_state(Pointer state, int i_segment, int i_token);
}
