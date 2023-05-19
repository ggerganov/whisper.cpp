package io.github.ggerganov.whispercpp;

import com.sun.jna.Native;

import java.io.InputStream;
import java.util.List;

public class WhisperJNI {
    static {
        System.setProperty("java.library.path",
                System.getProperty("java.library.path")
                        + ":" + System.getProperty("user.dir") + "/build/libs/whispercpp/shared"
//                        + ":" + System.getProperty("user.dir") + "/build/libs/whispercpp/static"
        );

        System.out.println("System.getProperty(\"java.library.path\"): " + System.getProperty("java.library.path"));

        // java.lang.UnsatisfiedLinkError: no whispercpp in java.library.path
//        System.loadLibrary("whispercpp");

        // java.lang.UnsatisfiedLinkError: Error looking up function 'getSystemInfo': /whisper.cpp/bindings/java/build/libs/whispercpp/shared/libwhispercpp.so: undefined symbol: getSystemInfo
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



    // Note: Running `./gradlew compileJava` will generate the `...WhisperJNI.h` file
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
