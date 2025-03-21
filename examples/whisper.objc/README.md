# whisper.objc

Minimal Obj-C application for automatic offline speech recognition.
The inference runs locally, on-device.

https://user-images.githubusercontent.com/1991296/197385372-962a6dea-bca1-4d50-bf96-1d8c27b98c81.mp4

Real-time transcription demo:

https://user-images.githubusercontent.com/1991296/204126266-ce4177c6-6eca-4bd9-bca8-0e46d9da2364.mp4

## Usage

This example uses the whisper.xcframework which needs to be built first using the following command:
```bash
./build_xcframework.sh
```

A model is also required to be downloaded and can be done using the following command:
```bash
./models/download-ggml-model.sh base.en
```

If you don't want to convert a Core ML model, you can skip this step by creating dummy model:
```bash
mkdir models/ggml-base.en-encoder.mlmodelc
```

## Core ML

Follow the [`Core ML support` section of readme](../../README.md#core-ml-support) to convert the model.
That is all the needs to be done to use the Core ML model in the app. The converted model is a
resource in the project and will be used if it is available.
