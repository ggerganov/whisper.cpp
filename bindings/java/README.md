# Java JNI bindings for Whisper

This package provides Java JNI bindings for whisper.cpp. They have been tested on:

  * <strike>Darwin (OS X) 12.6 on x64_64</strike>
  * Windows on x86_64

The "low level" bindings are in the `bindings/java` directory. The most simple usage is as follows:

```java
import io.github.ggerganov.whispercpp.Whisper;

public class Example {
     static {
          System.loadLibrary("whisper");
     }

     public static void main(String[] args) {
          String modelpath;
          double[] samples;


          try (Model model = new Whisper(modelpath)) {
               val context = model.createContext();
               context.process(samples);

               for (val segement in context.nextSegment()){
                    System.out.println(segment.getText());
               }

          } catch (Exeception e) {
               model.close();
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

Or `javac -h src/main/cpp -d build/classes/java/main src/main/java/io/github/ggerganov/whispercpp/WhisperJNI.java`

This will create/update [io_github_ggerganov_whispercpp_WhisperJNI.h](build/generated/sources/headers/java/main/io_github_ggerganov_whispercpp_WhisperJNI.h).






## License

The license for the Go bindings is the same as the license for the rest of the whisper.cpp project, which is the MIT License. See the `LICENSE` file for more details.

