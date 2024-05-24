import argparse
import torch
from whisper import load_model
from whisper.model import AudioEncoder
import os
from nncf import compress_weights, CompressWeightsMode
from openvino.frontend import FrontEndManager
from openvino.runtime import serialize
import shutil
from torch import Tensor
import torch.nn.functional as F

# Monkey path the whisper AudioEncoder.forward method to accept variable audio context size (n_mels)
def patched_forward(self, x: Tensor):
    """
    x : torch.Tensor, shape = (batch_size, n_mels, n_ctx)
        the mel spectrogram of the audio
    """
    x = F.gelu(self.conv1(x))
    x = F.gelu(self.conv2(x))
    x = x.permute(0, 2, 1)

    # Modify positional embedding shape to match the input mel shape
    x = (x + self.positional_embedding[0:x.shape[1], :]).to(x.dtype)

    for block in self.blocks:
        x = block(x)

    x = self.ln_post(x)
    return x

# Patch the method
AudioEncoder.forward = patched_forward

def convert_encoder(hparams, encoder, mname, n_audio_ctx, quantize_bits = 8):
    encoder.eval()

    mel = torch.zeros((1, hparams.n_mels, n_audio_ctx*2))

    onnx_folder = os.path.join(os.path.dirname(__file__), "onnx_encoder")

    #create a directory to store the onnx model, and other collateral that is saved during onnx export procedure
    if not os.path.isdir(onnx_folder):
        os.makedirs(onnx_folder)

    onnx_path = os.path.join(onnx_folder, "whisper_encoder.onnx")

    # Export the PyTorch model to ONNX
    torch.onnx.export(
        encoder,
        mel,
        onnx_path,
        input_names=["mel"],
        output_names=["output_features"]
    )

    # Convert ONNX to OpenVINO IR format using the frontend
    fem = FrontEndManager()
    onnx_fe = fem.load_by_framework("onnx")
    onnx_model = onnx_fe.load(onnx_path)
    ov_model = onnx_fe.convert(onnx_model)

    # Quantize the model using OpenVino NNCF (https://github.com/openvinotoolkit/nncf)
    if quantize_bits == 8:
        ov_model = compress_weights(ov_model, mode=CompressWeightsMode.INT8)
    if quantize_bits == 4:
        ov_model = compress_weights(ov_model, mode=CompressWeightsMode.INT4_SYMM)

    # Serialize the OpenVINO model to XML and BIN files
    serialize(ov_model, xml_path=os.path.join(os.path.dirname(__file__), "ggml-" + mname + "-encoder-openvino.xml"))

    # Cleanup
    if os.path.isdir(onnx_folder):
        shutil.rmtree(onnx_folder)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-m", "--model", type=str, help="model to convert (e.g. tiny, tiny.en, base, base.en, small, small.en, medium, medium.en, large-v1, large-v2, large-v3)", required=True)
    parser.add_argument("-ac", "--audio_context", type=int, help="number of audio context frames to set when converting the model (fixed value)", required=False, default=1500)
    parser.add_argument("-qb", "--quantize_bits", type=int, help="quantize model to 8 or 4 bits", required=False, default=None)
    args = parser.parse_args()

    if args.model not in ["tiny", "tiny.en", "base", "base.en", "small", "small.en", "medium", "medium.en", "large-v1", "large-v2", "large-v3"]:
        raise ValueError("Invalid model name")

    if args.quantize_bits is not None and args.quantize_bits not in [8, 4]:
        raise ValueError("Invalid quantization level, only 8 and 4 bit are supported")

    whisper = load_model(args.model).cpu()
    hparams = whisper.dims
    hparams.n_audio_ctx = args.audio_context
    encoder = whisper.encoder

    # Convert encoder to onnx
    convert_encoder(hparams, encoder, args.model, args.audio_context, args.quantize_bits)
