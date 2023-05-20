package io.github.ggerganov.whispercpp;

import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import io.github.ggerganov.whispercpp.params.WhisperJavaParams;

interface WhisperJavaJnaLibrary extends Library {
    WhisperJavaJnaLibrary instance = Native.load("whisper_java", WhisperJavaJnaLibrary.class);

    void whisper_java_default_params(int strategy);

    void whisper_java_free();

    void whisper_java_init_from_file(String modelPath);

    /**
     * Run the entire model: PCM -> log mel spectrogram -> encoder -> decoder -> text.
     * Not thread safe for same context
     * Uses the specified decoding strategy to obtain the text.
     */
    int whisper_java_full(Pointer ctx, /*WhisperJavaParams params, */float[] samples, int nSamples);
}
