package io.github.ggerganov.whispercpp;

import com.sun.jna.Pointer;
import io.github.ggerganov.whispercpp.params.WhisperJavaParams;
import io.github.ggerganov.whispercpp.params.WhisperSamplingStrategy;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;

/**
 * Before calling most methods, you must call `initContext(modelPath)` to initialise the `ctx` Pointer.
 */
public class WhisperCpp implements AutoCloseable {
    private WhisperCppJnaLibrary lib = WhisperCppJnaLibrary.instance;
    private WhisperJavaJnaLibrary javaLib = WhisperJavaJnaLibrary.instance;
    private Pointer ctx = null;

    public File modelDir() {
        String modelDirPath = System.getenv("XDG_CACHE_HOME");
        if (modelDirPath == null) {
            modelDirPath = System.getProperty("user.home") + "/.cache";
        }

        return new File(modelDirPath, "whisper");
    }

    /**
     * @param modelPath - absolute path, or just the name (eg: "base", "base-en" or "base.en")
     * @return a Pointer to the WhisperContext
     */
    void initContext(String modelPath) throws FileNotFoundException {
        if (ctx != null) {
            lib.whisper_free(ctx);
        }

        if (!modelPath.contains("/") && !modelPath.contains("\\")) {
            if (!modelPath.endsWith(".bin")) {
                modelPath = "ggml-" + modelPath.replace("-", ".") + ".bin";
            }

            modelPath = new File(modelDir(), modelPath).getAbsolutePath();
        }

        javaLib.whisper_java_init_from_file(modelPath);
        ctx = lib.whisper_init_from_file(modelPath);

        if (ctx == null) {
            throw new FileNotFoundException(modelPath);
        }
    }

    /**
     * Initialises `whisper_full_params` internally in whisper_java.cpp so JNA doesn't have to map everything.
     * `whisper_java_init_from_file()` calls `whisper_java_default_params(WHISPER_SAMPLING_GREEDY)` for convenience.
     */
    public void getDefaultJavaParams(WhisperSamplingStrategy strategy) {
        javaLib.whisper_java_default_params(strategy.ordinal());
//        return lib.whisper_full_default_params(strategy.value)
    }

// whisper_full_default_params was too hard to integrate with, so for now we use javaLib.whisper_java_default_params
//    fun getDefaultParams(strategy: WhisperSamplingStrategy): WhisperFullParams {
//        return lib.whisper_full_default_params(strategy.value)
//    }

    @Override
    public void close() {
        freeContext();
        System.out.println("Whisper closed");
    }

    private void freeContext() {
        if (ctx != null) {
            lib.whisper_free(ctx);
        }
    }

    /**
     * Run the entire model: PCM -> log mel spectrogram -> encoder -> decoder -> text.
     * Not thread safe for same context
     * Uses the specified decoding strategy to obtain the text.
     */
    public String fullTranscribe(/*WhisperJavaParams whisperParams,*/ float[] audioData) throws IOException {
        if (ctx == null) {
            throw new IllegalStateException("Model not initialised");
        }

        if (javaLib.whisper_java_full(ctx, /*whisperParams,*/ audioData, audioData.length) != 0) {
            throw new IOException("Failed to process audio");
        }

        int nSegments = lib.whisper_full_n_segments(ctx);

        StringBuilder str = new StringBuilder();

        for (int i = 0; i < nSegments; i++) {
            String text = lib.whisper_full_get_segment_text(ctx, i);
            System.out.println("Segment:" + text);
            str.append(text);
        }

        return str.toString().trim();
    }

//    public int getTextSegmentCount(Pointer ctx) {
//        return lib.whisper_full_n_segments(ctx);
//    }
//    public String getTextSegment(Pointer ctx, int index) {
//        return lib.whisper_full_get_segment_text(ctx, index);
//    }

    public String getSystemInfo() {
        return lib.whisper_print_system_info();
    }

    public int benchMemcpy(int nthread) {
        return lib.whisper_bench_memcpy(nthread);
    }

    public int benchGgmlMulMat(int nthread) {
        return lib.whisper_bench_ggml_mul_mat(nthread);
    }
}
