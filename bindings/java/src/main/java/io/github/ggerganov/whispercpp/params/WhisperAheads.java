package io.github.ggerganov.whispercpp.params;

import com.sun.jna.*;

import java.util.Arrays;
import java.util.List;

public class WhisperAheads extends Structure {
    public long n_heads;
    public Pointer heads;

    @Override
    protected List<String> getFieldOrder() {
        return Arrays.asList("n_heads", "heads");
    }
}
