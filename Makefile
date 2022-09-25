main: ggml.o main.o
	g++ -o main ggml.o main.o

ggml.o: ggml.c ggml.h
	gcc -O3 -mavx -mavx2 -mfma -mf16c -c ggml.c

main.o: main.cpp ggml.h
	g++ -O3 -std=c++11 -c main.cpp

# clean up the directory
clean:
	rm -f *.o main

# run the program
run: main
	./main

# download the following audio samples into folder "./samples":
.PHONY: samples
samples:
	@echo "Downloading samples..."
	mkdir -p samples
	@wget --quiet --show-progress -O samples/gb0.ogg https://upload.wikimedia.org/wikipedia/commons/2/22/George_W._Bush%27s_weekly_radio_address_%28November_1%2C_2008%29.oga
	@wget --quiet --show-progress -O samples/gb1.ogg https://upload.wikimedia.org/wikipedia/commons/1/1f/George_W_Bush_Columbia_FINAL.ogg
	@wget --quiet --show-progress -O samples/hp0.ogg https://upload.wikimedia.org/wikipedia/en/d/d4/En.henryfphillips.ogg
	@echo "Converting to 16-bit WAV ..."
	@ffmpeg -loglevel -0 -y -i samples/gb0.ogg -ar 16000 -ac 1 -c:a pcm_s16le samples/gb0.wav
	@ffmpeg -loglevel -0 -y -i samples/gb1.ogg -ar 16000 -ac 1 -c:a pcm_s16le samples/gb1.wav
	@ffmpeg -loglevel -0 -y -i samples/hp0.ogg -ar 16000 -ac 1 -c:a pcm_s16le samples/hp0.wav

.PHONY: tiny.en
tiny.en: main
	@echo "Downloading tiny.en (75 MB just once)"
	mkdir -p models
	@if [ ! -f models/ggml-tiny.en.bin ]; then \
		wget --quiet --show-progress -O models/ggml-tiny.en.bin https://ggml.ggerganov.com/ggml-model-whisper-tiny.en.bin ; \
	fi
	@echo "==============================================="
	@echo "Running tiny.en on all samples in ./samples ..."
	@echo "==============================================="
	@echo ""
	@for f in samples/*.wav; do \
		echo "----------------------------------------------" ; \
		echo "[+] Running base.en on $$f ... (run 'ffplay $$f' to listen)" ; \
	    echo "----------------------------------------------" ; \
		echo "" ; \
		./main -m models/ggml-tiny.en.bin -f $$f ; \
		echo "" ; \
	done

.PHONY: base.en
base.en: main
	@echo "Downloading base.en (142 MB just once)"
	mkdir -p models
	@if [ ! -f models/ggml-base.en.bin ]; then \
		wget --quiet --show-progress -O models/ggml-base.en.bin https://ggml.ggerganov.com/ggml-model-whisper-base.en.bin ; \
	fi
	@echo "==============================================="
	@echo "Running base.en on all samples in ./samples ..."
	@echo "==============================================="
	@echo ""
	@for f in samples/*.wav; do \
		echo "----------------------------------------------" ; \
		echo "[+] Running base.en on $$f ... (run 'ffplay $$f' to listen)" ; \
	    echo "----------------------------------------------" ; \
		echo "" ; \
		./main -m models/ggml-base.en.bin -f $$f ; \
		echo "" ; \
	done

.PHONY: small.en
small.en: main
	@echo "Downloading small.en (466 MB just once)"
	mkdir -p models
	@if [ ! -f models/ggml-small.en.bin ]; then \
		wget --quiet --show-progress -O models/ggml-small.en.bin https://ggml.ggerganov.com/ggml-model-whisper-small.en.bin ; \
	fi
	@echo "==============================================="
	@echo "Running small.en on all samples in ./samples ..."
	@echo "==============================================="
	@echo ""
	@for f in samples/*.wav; do \
		echo "----------------------------------------------" ; \
		echo "[+] Running base.en on $$f ... (run 'ffplay $$f' to listen)" ; \
	    echo "----------------------------------------------" ; \
		echo "" ; \
		./main -m models/ggml-small.en.bin -f $$f ; \
		echo "" ; \
	done

.PHONY: medium.en
medium.en: main
	@echo "Downloading medium.en (1.5 GB just once)"
	mkdir -p models
	@if [ ! -f models/ggml-medium.en.bin ]; then \
		wget --quiet --show-progress -O models/ggml-medium.en.bin https://ggml.ggerganov.com/ggml-model-whisper-medium.en.bin ; \
	fi
	@echo "==============================================="
	@echo "Running medium.en on all samples in ./samples ..."
	@echo "==============================================="
	@echo ""
	@for f in samples/*.wav; do \
		echo "----------------------------------------------" ; \
		echo "[+] Running base.en on $$f ... (run 'ffplay $$f' to listen)" ; \
	    echo "----------------------------------------------" ; \
		echo "" ; \
		./main -m models/ggml-medium.en.bin -f $$f ; \
		echo "" ; \
	done
