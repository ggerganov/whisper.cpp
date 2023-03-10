FROM ubuntu:18.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        ca-certificates \
        curl \
        ffmpeg \
        wget \
        make \
        vim \   
        build-essential \
        gcc-8 g++-8


RUN mkdir whisper/
COPY . whisper/
WORKDIR whisper
RUN make clean
RUN models/download-ggml-model.sh base.en
RUN models/download-ggml-model.sh medium.en
RUN models/download-ggml-model.sh large
RUN make
