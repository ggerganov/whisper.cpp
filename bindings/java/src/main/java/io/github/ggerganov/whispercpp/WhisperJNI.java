package io.github.ggerganov.whispercpp;

import com.sun.jna.Native;

import java.io.InputStream;
import java.util.List;

public class WhisperJNI {
    static {
//        System.loadLibrary("whispercpp");
        Native.register(WhisperJNI.class, "whispercpp");
    }

    private static WhisperJNI instance;

    private WhisperJNI() {
    }

    public static synchronized WhisperJNI getInstance() {
        if (instance == null) {
            instance = new WhisperJNI();
        }
        return instance;
    }

    public native long initContextFromInputStream(InputStream inputStream);
//    public native long initContextFromAsset(AssetManager assetManager, String assetPath)l
    public native long initContext(String modelPath);
    public native void freeContext(long contextPtr);
    public native void fullTranscribe(long contextPtr, float[] audioData);
    public native int getTextSegmentCount(long contextPtr);
    public native String getTextSegment(long contextPtr, int index);
    public native String getSystemInfo();
    public native String benchMemcpy(int nthread);
    public native String benchGgmlMulMat(int nthread);

}
