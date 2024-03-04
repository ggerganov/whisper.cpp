package io.github.ggerganov.whispercpp.params;

import com.sun.jna.*;

import java.util.Arrays;
import java.util.List;

/**
 * Parameters for the whisper_init_from_file_with_params() function.
 * If you change the order or add new parameters, make sure to update the default values in whisper.cpp:
 * whisper_context_default_params()
 */
public class WhisperContextParams extends Structure {

    public WhisperContextParams(Pointer p) {
        super(p);
    }

    /** Use GPU for inference Number (default = true) */
    public CBool use_gpu;

    /** Use GPU for inference Number (default = true) */
    public void useGpu(boolean enable) {
        use_gpu = enable ? CBool.TRUE : CBool.FALSE;
    }

    @Override
    protected List<String> getFieldOrder() {
        return Arrays.asList("use_gpu");
    }
}
