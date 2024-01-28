A sample Android app using [whisper.cpp](https://github.com/ggerganov/whisper.cpp/) to do voice-to-text transcriptions.

To use:

1. Select a model from the [whisper.cpp repository](https://github.com/ggerganov/whisper.cpp/tree/master/models).[^1]
2. Copy the model to the "app/src/main/assets/models" folder.
3. Select a sample audio file (for example, [jfk.wav](https://github.com/ggerganov/whisper.cpp/raw/master/samples/jfk.wav)).
4. Copy the sample to the "app/src/main/assets/samples" folder.
5. Select the "release" active build variant, and use Android Studio to run and deploy to your device.
[^1]: I recommend the tiny or base models for running on an Android device.

(PS: Do not move this android project folder individually to other folders, because this android project folder depends on the files of the whole project.)

<img width="300" alt="image" src="https://user-images.githubusercontent.com/1670775/221613663-a17bf770-27ef-45ab-9a46-a5f99ba65d2a.jpg">

## CLBlast

> [!NOTE]
> Note: OpenCL does not have the same level of support as CUDA or Metal.

Build CLBlast.

```
# In path/to/CLBlast (we assume OpenCL-Headers relative location)
$ANDROID_SDK_PATH/cmake/3.22.1/bin/cmake .. \
    -DCMAKE_SYSTEM_NAME=Android \
    -DCMAKE_SYSTEM_VERSION=33 \
    -DCMAKE_ANDROID_ARCH_ABI=arm64-v8a \
    -DCMAKE_ANDROID_NDK=$ANDROID_NDK_PATH \
    -DCMAKE_ANDROID_STL_TYPE=c++_static \
    -DOPENCL_ROOT=$(readlink -f ../../OpenCL-Headers) \
    -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=BOTH \
    -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=BOTH

# Build libclblast.so
make -j4
```

Pull `libGLES_mali.so` to `libOpenCL.so`.

```bash
# In path/to/whisper.android
mkdir lib/src/main/jniLibs/arm64-v8a
adb pull /system/vendor/lib64/egl/libGLES_mali.so lib/src/main/jniLibs/arm64-v8a/libOpenCL.so
```

In lib/build.gradle, set `GGML_HOME` to the location of GGML, as well as
required options for turning on CLBlast.

```gradle
cmake {
    arguments "-DGGML_HOME=/path/to/ggml",
        "-DGGML_CLBLAST=ON",
        "-DOPENCL_ROOT=/path/to/OpenCL-Headers",
        "-DOPENCL_LIB=/path/to/libOpenCL.so",
        "-DCLBLAST_HOME=/path/to/clblast",
        "-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=BOTH",
        "-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=BOTH"
}
```

