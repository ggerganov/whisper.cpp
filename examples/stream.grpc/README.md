# stream.grpc

This is a naive example of a bidirectional, asynchronous gRPC streaming interface to the whisper.cpp streaming transcription functionlity (similar to the main stream example, but callable via gRPC with a given host and port endpoint):

```bash
./stream.grpc -m ./models/ggml-base.en.bin -t 2 --step 0 --length 5000 -gh 0.0.0.0 -gp 50051
```

## Building

The `stream.grpc` tool depends on the C++ version of the gRPC library and tools (e.g., protobuf compiler) being installed to do the build.  It can be installed via the instructions at the following URL:

https://grpc.io/docs/languages/cpp/quickstart/

Once that is installed, the build can be done with the CMake build system by setting the WHISPER_GRPC build variable in the build command:

```bash
make -B build -DWHISPER_BUILD_EXAMPLES=1 -DWHISPER_GRPC=1 && cmake --build build -j 4 --config Release
```