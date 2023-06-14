package io.github.ggerganov.whispercpp;

import com.sun.jna.Structure;
import com.sun.jna.ptr.PointerByReference;
import io.github.ggerganov.whispercpp.ggml.GgmlType;
import io.github.ggerganov.whispercpp.WhisperModel;

import java.util.List;

public class WhisperContext extends Structure {
    int t_load_us = 0;
    int t_start_us = 0;

    /** weight type (FP32 / FP16 / QX) */
    GgmlType wtype = GgmlType.GGML_TYPE_F16;
    /** intermediate type (FP32 or FP16) */
    GgmlType itype = GgmlType.GGML_TYPE_F16;

//    WhisperModel model;
    public PointerByReference model;
//    whisper_vocab vocab;
//    whisper_state * state = nullptr;
    public PointerByReference vocab;
    public PointerByReference state;

    /** populated by whisper_init_from_file() */
    String path_model;

//    public static class ByReference extends WhisperContext implements Structure.ByReference {
//    }
//
//    public static class ByValue extends WhisperContext implements Structure.ByValue {
//    }
//
//    @Override
//    protected List<String> getFieldOrder() {
//        return List.of("t_load_us", "t_start_us", "wtype", "itype", "model", "vocab", "state", "path_model");
//    }
}
