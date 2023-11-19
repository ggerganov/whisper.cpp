import ctypes

import numpy as np
import whisper_cpp_wrapper


class WhisperCpp:
    """Wrapper around whisper.cpp, which is a C++ implementation of the Whisper
    speech recognition model.
    """

    def __init__(self, model: str, params=None) -> None:
        self.ctx = whisper_cpp_wrapper.whisper_init_from_file(model.encode("utf-8"))

    def transcribe(self, audio: np.ndarray, params=None) -> str:
        """Transcribe audio using the given parameters.

        Any is whisper_cpp.WhisperParams, but we can't import that here
        because it's a C++ class.
        """
        # Set the default parameters if none are given
        if not params:
            self.params = whisper_cpp_wrapper.whisper_full_default_params(
                whisper_cpp_wrapper.WHISPER_SAMPLING_GREEDY  # 0, faster
            )
        else:
            self.params = params

        # Run the model
        whisper_cpp_audio = audio.ctypes.data_as(ctypes.POINTER(ctypes.c_float))
        result = whisper_cpp_wrapper.whisper_full(
            self.ctx, self.params, whisper_cpp_audio, len(audio)
        )
        if result != 0:
            raise Exception(f"Error from whisper.cpp: {result}")

        # Get the text
        n_segments = whisper_cpp_wrapper.whisper_full_n_segments((self.ctx))
        text = [
            whisper_cpp_wrapper.whisper_full_get_segment_text((self.ctx), i)
            for i in range(n_segments)
        ]

        return text[0].decode("utf-8")

    def __del__(self):
        """
        Free the C++ object when this Python object is garbage collected.
        """
        whisper_cpp_wrapper.whisper_free(self.ctx)
