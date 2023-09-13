#!/usr/bin/env python3

import time
import os
import numpy as np

import torch
import torch.nn as nn
from torch import Tensor
from typing import Dict, Iterable, Optional

from whisper import load_model
from whisper.model import Whisper, TextDecoder, AudioEncoder, ModelDimensions

torch.set_printoptions(sci_mode=False)
np.set_printoptions(suppress=True)

import ane


class TextDecoderANE(TextDecoder):
	def __init__(
		self, n_vocab: int, n_ctx: int, n_state: int, n_head: int, n_layer: int,
		path = "data/decoder-tiny.anec"
	):
		super().__init__(n_vocab, n_ctx, n_state, n_head, n_layer)
		self.path = path
		self.ane_model = ane.Model(path)

	def forward(self, x: Tensor, xa: Tensor, kv_cache: Optional[dict] = None):
		"""
		x : torch.LongTensor, shape = (batch_size, <= n_ctx)
		    the text tokens
		xa : torch.Tensor, shape = (batch_size, n_mels, n_audio_ctx)
		    the encoded audio features to be attended on
		"""

		offset = next(iter(kv_cache.values())).shape[1] if kv_cache else 0
		x = self.token_embedding(x) + self.positional_embedding[offset : offset + x.shape[-1]]
		x = x.to(xa.dtype)

		block = self.blocks[0]

		src2 = x + block.attn(block.attn_ln(x), mask=self.mask, kv_cache=kv_cache)[0]
		src1 = xa
		src0 = block.cross_attn_ln(src2) - block.cross_attn_ln.bias

		src2 = np.expand_dims(src2.detach().clone().cpu().numpy().astype(np.float16), 0)
		src1 = np.expand_dims(src1.detach().clone().cpu().numpy().astype(np.float16), 0)
		src0 = np.expand_dims(src0.detach().clone().cpu().numpy().astype(np.float16), 0)

		pred = self.ane_model.predict([src0, src1, src2])[0]
		x = torch.from_numpy(pred.astype(np.float32)).squeeze(0)

		logits = x @ torch.transpose(self.token_embedding.weight.to(x.dtype), 0, 1)
		return logits


class WhisperANE(Whisper):
	def __init__(self, dims: ModelDimensions):
		super().__init__(dims)

		self.encoder = AudioEncoder(
		    self.dims.n_mels,
		    self.dims.n_audio_ctx,
		    self.dims.n_audio_state,
		    self.dims.n_audio_head,
		    self.dims.n_audio_layer,
		)
		self.decoder = TextDecoderANE(
		    self.dims.n_vocab,
		    self.dims.n_text_ctx,
		    self.dims.n_text_state,
		    self.dims.n_text_head,
		    self.dims.n_text_layer,
		)

	def forward(self, mel: torch.Tensor, tokens: torch.Tensor) -> Dict[str, torch.Tensor]:
		return self.decoder(tokens, self.encoder(mel))

	def install_kv_cache_hooks(self, cache: Optional[dict] = None):
		cache = {**cache} if cache is not None else {}
		hooks = []

		def save_to_cache(module, _, output):
			if module not in cache or output.shape[3] > self.decoder.positional_embedding.shape[0]:
				cache[module] = output  # save as-is, for the first token or cross attention
			else:
				cache[module] = torch.cat([cache[module], output], dim=3).detach()
			return cache[module]

		def install_hooks(layer: nn.Module):
			if isinstance(layer, MultiHeadAttentionANE):
				hooks.append(layer.key.register_forward_hook(save_to_cache))
				hooks.append(layer.value.register_forward_hook(save_to_cache))

		self.decoder.apply(install_hooks)
		return cache, hooks


if __name__ == "__main__":
	whisper = load_model("tiny").cpu()
	hparams = whisper.dims
	print(hparams)

	whisperANE = WhisperANE(hparams).eval().cpu()
	whisperANE.load_state_dict(whisper.state_dict())

	audio_data = torch.randn((1, 1500, 384)).to(torch.float32)
	token_data = torch.randint(50257, (1, 1)).to(torch.int32)

	tic = time.time()
	pred = whisperANE.decoder(token_data, audio_data).detach().cpu()
	toc = time.time()
	d1 = toc - tic
	print("ANE:", pred)
	print ("ANE: %.20f" % d1)

	tic = time.time()
	orig = whisper.decoder(token_data, audio_data).detach().cpu()
	toc = time.time()
	d2 = toc - tic
	print("CPU:", orig)
	print("CPU: %.20f" % d2)
	print("speedup: %.20f" % (d2 / d1))

	print("diff:", orig - pred)
