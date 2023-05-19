package io.github.ggerganov.whispercpp;

import java.io.InputStream;

public class Whisper {
    private WhisperJnaLibrary lib = WhisperJnaLibrary.instance;

    public long initContextFromInputStream(InputStream inputStream) {
        lib.whisper_init(loader)
    }

    //    public long initContextFromAsset(AssetManager assetManager, String assetPath)l
    public long initContext(String modelPath) {

    }
    public void freeContext(long contextPtr) {

    }
    public void fullTranscribe(long contextPtr, float[] audioData) {

    }
    public int getTextSegmentCount(long contextPtr) {

    }
    public String getTextSegment(long contextPtr, int index) {

    }
    public String getSystemInfo() {

    }
    public String benchMemcpy(int nthread) {

    }
    public String benchGgmlMulMat(int nthread) {

    }
}
