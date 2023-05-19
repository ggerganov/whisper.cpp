# Java JNI bindings for Whisper

This package provides Java JNI bindings for whisper.cpp. They have been tested on:

  * <strike>Darwin (OS X) 12.6 on x64_64</strike>
  * Windows on x86_64

The "low level" bindings are in the `bindings/java` directory. The most simple usage is as follows:

```java
import io.github.ggerganov.whispercpp.Whisper;

public class Example {
     static {
         Native.register(WhisperJNI.class, "whispercpp");
     }

     public static void main(String[] args) {
          String modelpath;
          WhisperJNI whisper = WhisperJNI.getInstance();
          double[] samples = getAudio();
          
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
./gradlew build
```

These are some of the tasks that are executed and can also be invoked directly: 
- `./gradlew compileJava`: also generates `build/generated/.../io_...WhisperJNI.h`
- `./gradlew whispercppSharedLibrary`: generate `build/libs/whispercpp/shared/libwhispercpp.so`


## License

The license for the Go bindings is the same as the license for the rest of the whisper.cpp project, which is the MIT License. See the `LICENSE` file for more details.

