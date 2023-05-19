package io.github.ggerganov.whispercpp;

import static org.assertj.core.api.Assertions.*;

import com.sun.jna.Native;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

public class WhisperJnaLibraryTest {

    @BeforeAll
    static void loadLibrary() {
//        System.setProperty("jna.nounpack", "false");
//        System.setProperty("jna.noclasspath", "false");

        System.setProperty("java.library.path",
                System.getProperty("java.library.path")
                        + ":" + System.getProperty("user.dir") + "/../../build/bin/Release/whisper-bin-x64"
//                        + ":" + System.getProperty("user.dir") + "/build/libs/whispercpp/static"
        );

        Native.register(WhisperJnaLibrary.class, "whisper");

//        System.load(System.getProperty("user.dir") + "/build/libs/whispercpp/shared/libwhispercpp.so");
    }

    @Test
    void test_whisper_print_system_info() {
//        System.out.println("testing...");
//        String systemInfo = WhisperJNI.getInstance().getSystemInfo();
//        assertThat(systemInfo).isEqualTo("getSystemInfo");

        String systemInfo = WhisperJnaLibrary.instance.whisper_print_system_info();
        assertThat(systemInfo).isEqualTo("asdf");
    }
}
