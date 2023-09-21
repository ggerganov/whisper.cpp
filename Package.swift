// swift-tools-version:5.7
import PackageDescription

let package = Package(
    name: "whisper.cpp",
    platforms: [
        .iOS(.v11),
        .macOS(.v10_13),
        .watchOS(.v4),
        .tvOS(.v11)
    ],
    products: [
        .library(
            name: "whispercpp",
            targets: ["whispercpp"]
        )
    ],
    targets: [
        .target(
            name: "whispercpp",
            dependencies: [],
            path: ".",
            exclude: [
                "bindings",
                "cmake",
                "coreml",
                "examples",
                "extra",
                "models",
                "samples",
                "tests",
                "CMakeLists.txt",
                "ggml-cuda.cu",
                "ggml-cuda.h",
                "ggml-opencl.c",
                "ggml-opencl.h",
                "Makefile"
            ],
            sources: [
                "whisper.cpp",
                "ggml.c"
            ],
            publicHeadersPath: "swiftpm"
        ),

    ],
    cxxLanguageStandard: .cxx11
)
