#
# Audio samples
#

.PHONY: build
build:
	cmake -B build
	cmake --build build --config Release

# download a few audio samples into folder "./samples":
.PHONY: samples
samples:
	@echo "Downloading samples..."
	@mkdir -p samples
	@wget --quiet --show-progress -O samples/gb0.ogg https://upload.wikimedia.org/wikipedia/commons/2/22/George_W._Bush%27s_weekly_radio_address_%28November_1%2C_2008%29.oga
	@wget --quiet --show-progress -O samples/gb1.ogg https://upload.wikimedia.org/wikipedia/commons/1/1f/George_W_Bush_Columbia_FINAL.ogg
	@wget --quiet --show-progress -O samples/hp0.ogg https://upload.wikimedia.org/wikipedia/en/d/d4/En.henryfphillips.ogg
	@wget --quiet --show-progress -O samples/mm1.wav https://cdn.openai.com/whisper/draft-20220913a/micro-machines.wav
	@wget --quiet --show-progress -O samples/a13.mp3 https://upload.wikimedia.org/wikipedia/commons/transcoded/6/6f/Apollo13-wehaveaproblem.ogg/Apollo13-wehaveaproblem.ogg.mp3
	@wget --quiet --show-progress -O samples/diffusion2023-07-03.flac https://archive.org/download/diffusion2023-07-03/diffusion2023-07-03.flac
	@echo "Converting to 16-bit WAV ..."
	@ffmpeg -loglevel -0 -y -i samples/gb0.ogg -ar 16000 -ac 1 -c:a pcm_s16le samples/gb0.wav
	@ffmpeg -loglevel -0 -y -i samples/gb1.ogg -ar 16000 -ac 1 -c:a pcm_s16le samples/gb1.wav
	@ffmpeg -loglevel -0 -y -i samples/hp0.ogg -ar 16000 -ac 1 -c:a pcm_s16le samples/hp0.wav
	@rm samples/*.ogg
	@ffmpeg -loglevel -0 -y -i samples/mm1.wav -ar 16000 -ac 1 -c:a pcm_s16le samples/mm0.wav
	@rm samples/mm1.wav
	@ffmpeg -loglevel -0 -y -i samples/a13.mp3 -ar 16000 -ac 1 -c:a pcm_s16le -ss 00:00:00 -to 00:00:30 samples/a13.wav
	@rm samples/a13.mp3
	@ffmpeg -loglevel -0 -y -i samples/diffusion2023-07-03.flac -ar 16000 -ac 1 -c:a pcm_s16le samples/diffusion2023-07-03.wav
	@rm samples/diffusion2023-07-03.flac

#
# Models
#

# if not already downloaded, the following targets download the specified model and
# runs it on all samples in the folder "./samples":

.PHONY: tiny.en
.PHONY: tiny
.PHONY: base.en
.PHONY: base
.PHONY: small.en
.PHONY: small
.PHONY: medium.en
.PHONY: medium
.PHONY: large-v1
.PHONY: large-v2
.PHONY: large-v3
.PHONY: large-v3-turbo

tiny.en tiny base.en base small.en small medium.en medium large-v1 large-v2 large-v3 large-v3-turbo:
	bash ./models/download-ggml-model.sh $@
	cmake -B build
	cmake --build build --config Release
	@echo ""
	@echo "==============================================="
	@echo "Running $@ on all samples in ./samples ..."
	@echo "==============================================="
	@echo ""
	@for f in samples/*.wav; do \
		echo "----------------------------------------------" ; \
		echo "[+] Running $@ on $$f ... (run 'ffplay $$f' to listen)" ; \
	    echo "----------------------------------------------" ; \
		echo "" ; \
		./build/bin/whisper-cli -m models/ggml-$@.bin -f $$f ; \
		echo "" ; \
	done
