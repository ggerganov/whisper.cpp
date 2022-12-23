A sample SwiftUI app using [whisper.cpp](https://github.com/ggerganov/whisper.cpp/) to do voice-to-text transcriptions.
See also: [whisper.objc](https://github.com/ggerganov/whisper.cpp/tree/master/examples/whisper.objc).

To use:

1. Select a model from the [whisper.cpp repository](https://github.com/ggerganov/whisper.cpp/tree/master/models).[^1]
2. Add the model to "whisper.swiftui.demo/Resources/models" via Xcode.
3. Select a sample audio file (for example, [jfk.wav](https://github.com/ggerganov/whisper.cpp/raw/master/samples/jfk.wav)).
4. Add the model to "whisper.swiftui.demo/Resources/samples" via Xcode.
5. Select the "release" build configuration under "Run", then deploy and run to your device.

[^1]: I recommend the tiny, base or small models for running on an iOS device.
