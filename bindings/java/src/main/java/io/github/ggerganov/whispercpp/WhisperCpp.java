package io.github.ggerganov.whispercpp;

import com.sun.jna.Native;
import com.sun.jna.Pointer;
import io.github.ggerganov.whispercpp.params.WhisperFullParams;
import io.github.ggerganov.whispercpp.params.WhisperSamplingStrategy;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;

/**
 * Before calling most methods, you must call `initContext(modelPath)` to initialise the `ctx` Pointer.
 */
public class WhisperCpp implements AutoCloseable {
    private WhisperCppJnaLibrary lib = WhisperCppJnaLibrary.instance;
    private Pointer ctx = null;
    private Pointer greedyPointer = null;
    private Pointer beamPointer = null;

    public File modelDir() {
        String modelDirPath = System.getenv("XDG_CACHE_HOME");
        if (modelDirPath == null) {
            modelDirPath = System.getProperty("user.home") + "/.cache";
        }

        return new File(modelDirPath, "whisper");
    }

    /**
     * @param modelPath - absolute path, or just the name (eg: "base", "base-en" or "base.en")
     */
    public void initContext(String modelPath) throws FileNotFoundException {
        if (ctx != null) {
            lib.whisper_free(ctx);
        }

        if (!modelPath.contains("/") && !modelPath.contains("\\")) {
            if (!modelPath.endsWith(".bin")) {
                modelPath = "ggml-" + modelPath.replace("-", ".") + ".bin";
            }

            modelPath = new File(modelDir(), modelPath).getAbsolutePath();
        }

        ctx = lib.whisper_init_from_file(modelPath);

        if (ctx == null) {
            throw new FileNotFoundException(modelPath);
        }
    }

    /**
     * Provides default params which can be used with `whisper_full()` etc.
     * Because this function allocates memory for the params, the caller must call either:
     * - call `whisper_free_params()`
     * - `Native.free(Pointer.nativeValue(pointer));`
     *
     * @param strategy - GREEDY
     */
    public WhisperFullParams getFullDefaultParams(WhisperSamplingStrategy strategy) {
        Pointer pointer;

        // whisper_full_default_params_by_ref allocates memory which we need to delete, so only create max 1 pointer for each strategy.
        if (strategy == WhisperSamplingStrategy.WHISPER_SAMPLING_GREEDY) {
            if (greedyPointer == null) {
                greedyPointer = lib.whisper_full_default_params_by_ref(strategy.ordinal());
            }
            pointer = greedyPointer;
        } else {
            if (beamPointer == null) {
                beamPointer = lib.whisper_full_default_params_by_ref(strategy.ordinal());
            }
            pointer = beamPointer;
        }

        WhisperFullParams params = new WhisperFullParams(pointer);
        params.read();
        return params;
    }

    @Override
    public void close() {
        freeContext();
        freeParams();
        System.out.println("Whisper closed");
    }

    private void freeContext() {
        if (ctx != null) {
            lib.whisper_free(ctx);
        }
    }

    private void freeParams() {
        if (greedyPointer != null) {
            Native.free(Pointer.nativeValue(greedyPointer));
            greedyPointer = null;
        }
        if (beamPointer != null) {
            Native.free(Pointer.nativeValue(beamPointer));
            beamPointer = null;
        }
    }

    /**
     * Run the entire model: PCM -> log mel spectrogram -> encoder -> decoder -> text.
     * Not thread safe for same context
     * Uses the specified decoding strategy to obtain the text.
     */
    public String fullTranscribe(WhisperFullParams whisperParams, float[] audioData) throws IOException {
        if (ctx == null) {
            throw new IllegalStateException("Model not initialised");
        }

        if (lib.whisper_full(ctx, whisperParams, audioData, audioData.length) != 0) {
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
