# Java JNI bindings for Whisper

This package provides Java JNI bindings for whisper.cpp. They have been tested on:

  * <strike>Darwin (OS X) 12.6 on x64_64</strike>
  * Ubuntu on x86_64
  * Windows on x86_64

The "low level" bindings are in `WhisperCppJnaLibrary` and `WhisperJavaJnaLibrary` which caches `whisper_full_params` and `whisper_context` in `whisper_java.cpp`. 

There are a lot of classes in the `callbacks`, `ggml`, `model` and `params` directories but most of them have not been tested. 

The most simple usage is as follows:

```java
import io.github.ggerganov.whispercpp.WhisperCpp;

public class Example {

    public static void main(String[] args) {
        String modelpath;
        WhisperCpp whisper = new WhisperCpp();
        // By default, models are loaded from ~/.cache/whisper/ and are usually named "ggml-${name}.bin"
        // or you can provide the absolute path to the model file.
        whisper.initContext("base.en"); 
        
        long context = whisper.initContext(modelpath);
        try {
            whisper.fullTranscribe(context, samples);
            
            int segmentCount = whisper.getTextSegmentCount(context);
            for (int i = 0; i < segmentCount; i++) {
                String text = whisper.getTextSegment(context, i);
                System.out.println(segment.getText());
            }
        } finally {
             whisper.freeContext(context);
        }
     }
}
```

## Building & Testing

In order to build, you need to have the JDK 8 or higher installed. Run the tests with:

```bash
git clone https://github.com/ggerganov/whisper.cpp.git
cd whisper.cpp/bindings/java

mkdir build
pushd build
cmake ..
cmake --build .
popd

./gradlew build
```

## License

The license for the Go bindings is the same as the license for the rest of the whisper.cpp project, which is the MIT License. See the `LICENSE` file for more details.

