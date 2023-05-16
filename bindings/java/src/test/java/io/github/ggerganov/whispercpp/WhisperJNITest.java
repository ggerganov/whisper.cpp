package io.github.ggerganov.whispercpp;

import static org.assertj.core.api.Assertions.*;

import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

import java.util.List;

public class WhisperJNITest {
    @BeforeAll
    static void loadLibrary() {
//        System.setProperty("jna.nounpack", "false");
//        System.setProperty("jna.noclasspath", "false");

        System.setProperty("java.library.path",
                System.getProperty("java.library.path")
                        + ":" + System.getProperty("user.dir") + "/build/libs/whispercpp/shared"
                        + ":" + System.getProperty("user.dir") + "/build/libs/whispercpp/static");

        System.load(System.getProperty("user.dir") + "/build/libs/whispercpp/shared/libwhispercpp.so");
    }

    @Test
    void test() {
        System.out.println("testing...");
        String systemInfo = WhisperJNI.getInstance().getSystemInfo();
        assertThat(systemInfo).isEqualTo("");
    }
}
