package io.github.ggerganov.whispercpp;

import static org.junit.jupiter.api.Assertions.*;

import io.github.ggerganov.whispercpp.params.WhisperJavaParams;
import io.github.ggerganov.whispercpp.params.WhisperSamplingStrategy;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import javax.sound.sampled.AudioInputStream;
import javax.sound.sampled.AudioSystem;
import java.io.File;
import java.io.FileNotFoundException;

class WhisperCppTest {
    private static WhisperCpp whisper = new WhisperCpp();
    private static boolean modelInitialised = false;

    @BeforeAll
    static void init() throws FileNotFoundException {
        // By default, models are loaded from ~/.cache/whisper/ and are usually named "ggml-${name}.bin"
        // or you can provide the absolute path to the model file.
        String modelName = "base.en";
        try {
            whisper.initContext(modelName);
            whisper.getDefaultJavaParams(WhisperSamplingStrategy.WHISPER_SAMPLING_GREEDY);
//            whisper.getDefaultJavaParams(WhisperSamplingStrategy.WHISPER_SAMPLING_BEAM_SEARCH);
            modelInitialised = true;
        } catch (FileNotFoundException ex) {
            System.out.println("Model " + modelName + " not found");
        }
    }

    @Test
    void testGetDefaultJavaParams() {
        // When
        whisper.getDefaultJavaParams(WhisperSamplingStrategy.WHISPER_SAMPLING_BEAM_SEARCH);

        // Then if it doesn't throw we've connected to whisper.cpp
    }

    @Test
    void testFullTranscribe() throws Exception {
        if (!modelInitialised) {
            System.out.println("Model not initialised, skipping test");
            return;
        }

        // Given
        File file = new File(System.getProperty("user.dir"), "../../samples/jfk.wav");
        AudioInputStream audioInputStream = AudioSystem.getAudioInputStream(file);

        byte[] b = new byte[audioInputStream.available()];
        float[] floats = new float[b.length / 2];

        try {
            audioInputStream.read(b);

            for (int i = 0, j = 0; i < b.length; i += 2, j++) {
                int intSample = (int) (b[i + 1]) << 8 | (int) (b[i]) & 0xFF;
                floats[j] = intSample / 32767.0f;
            }

            // When
            String result = whisper.fullTranscribe(/*params,*/ floats);

            // Then
            System.out.println(result);
            assertEquals("And so my fellow Americans, ask not what your country can do for you, " +
                    "ask what you can do for your country.",
                    result);
        } finally {
            audioInputStream.close();
        }
    }
}
