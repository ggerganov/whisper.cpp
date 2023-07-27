# whisper.objc

Minimal Obj-C application for automatic offline speech recognition.
The inference runs locally, on-device.

https://user-images.githubusercontent.com/1991296/197385372-962a6dea-bca1-4d50-bf96-1d8c27b98c81.mp4

Real-time transcription demo:

https://user-images.githubusercontent.com/1991296/204126266-ce4177c6-6eca-4bd9-bca8-0e46d9da2364.mp4

## Usage

```java
git clone https://github.com/ggerganov/whisper.cpp
open whisper.cpp/examples/whisper.objc/whisper.objc.xcodeproj/

// If you don't want to convert a Core ML model, you can skip this step by create dummy model
mkdir models/ggml-base.en-encoder.mlmodelc
```

Make sure to build the project in `Release`:

<img width="947" alt="image" src="https://user-images.githubusercontent.com/1991296/197382607-9e1e6d1b-79fa-496f-9d16-b71dc1535701.png">

Also, don't forget to add the `-DGGML_USE_ACCELERATE` compiler flag for `ggml.c` in Build Phases.
This can significantly improve the performance of the transcription:

<img width="1072" alt="image" src="https://user-images.githubusercontent.com/1991296/208511239-8d7cdbd1-aa48-41b5-becd-ca288d53cc07.png">

If you want to enable Core ML support, you can add the `-DWHISPER_USE_COREML -DWHISPER_COREML_ALLOW_FALLBACK` compiler flag for `whisper.cpp` in Build Phases:

<img width="1072" alt="image" src="https://github.com/ggerganov/whisper.cpp/assets/3001525/103e8f57-6eb6-490d-a60c-f6cf6c319324">

Then follow the [`Core ML support` section of readme](../../README.md#core-ml-support) for convert the model.

In this project, it also added `-O3 -DNDEBUG` to `Other C Flags`, but adding flags to app proj is not ideal in real world (applies to all C/C++ files), consider splitting xcodeproj in workspace in your own project.
