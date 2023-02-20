FROM ubuntu

RUN apt-get update -y && \
    apt-get install cmake g++ make git wget libopenblas-base libopenblas-dev -y && \
    mkdir -p /app/whisper 
WORKDIR /app/whisper
COPY . .

RUN cmake . -DWHISPER_SUPPORT_OPENBLAS=ON && make
RUN cd /app/whisper/models && \
    ./download-ggml-model.sh large && \
    mv /app/whisper/models/ggml-large.bin /app/whisper/models/ggml-main.bin
