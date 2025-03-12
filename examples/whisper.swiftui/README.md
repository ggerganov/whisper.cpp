# whisper.cpp/examples/whisper.swiftui

A sample SwiftUI app using [whisper.cpp](https://github.com/ggerganov/whisper.cpp/) to do voice-to-text transcriptions.
See also: [whisper.objc](https://github.com/ggerganov/whisper.cpp/tree/master/examples/whisper.objc).

### Building
 First whisper.cpp need to be built and a XCFramework needs to be created. This can be done by running
 the following script from the whisper.cpp project root:
 ```console
 $ ./build-xcframework.sh
 ```

Note: if you get the error "iphoneos is not an iOS SDK" then you probably need to run this command first:
```console
sudo xcode-select -switch /Applications/Xcode.app/Contents/Developer
```

 Open `whisper.swiftui.xcodeproj` project in Xcode and you should be able to build and run the app on
 a simulator or a real device.

 To use the framework with a different project, the XCFramework can be added to the project by
 adding `build-apple/whisper.xcframework` by dragging and dropping it into the project navigator, or
 by manually selecting the framework in the "Frameworks, Libraries, and Embedded Content" section
 of the project settings.

### Usage

1. Select a model from the [whisper.cpp repository](https://github.com/ggerganov/whisper.cpp/tree/master/models).[^1]
2. Add the model to `whisper.swiftui.demo/Resources/models` **via Xcode**.
3. Select a sample audio file (for example, [jfk.wav](https://github.com/ggerganov/whisper.cpp/raw/master/samples/jfk.wav)).
4. Add the sample audio file to `whisper.swiftui.demo/Resources/samples` **via Xcode**.
5. Select the "Release" [^2] build configuration under "Run", then deploy and run to your device.

**Note:** Pay attention to the folder path: `whisper.swiftui.demo/Resources/models` is the appropriate directory to place resources whilst `whisper.swiftui.demo/Models` is related to actual code.

[^1]: I recommend the tiny, base or small models for running on an iOS device.

[^2]: The `Release` build can boost performance of transcription. In this project, it also added `-O3 -DNDEBUG` to `Other C Flags`, but adding flags to app proj is not ideal in real world (applies to all C/C++ files), consider splitting xcodeproj in workspace in your own project.

![image](https://user-images.githubusercontent.com/1991296/212539216-0aef65e4-f882-480a-8358-0f816838fd52.png)
