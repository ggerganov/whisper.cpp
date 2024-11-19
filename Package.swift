// swift-tools-version:5.5

import PackageDescription

let package = Package(
    name: "whisper",
    platforms: [
        .macOS(.v12),
        .iOS(.v14),
        .watchOS(.v4),
        .tvOS(.v14)
    ],
    products: [
        .library(name: "whisper", targets: ["whisper"]),
    ],
    targets: [
        .target(
            name: "whisper",
            path: ".",
            exclude: [
               "build",
               "bindings",
               "cmake",
               "examples",
               "scripts",
               "models",
               "samples",
               "tests",
               "CMakeLists.txt",
               "Makefile",
               "ggml/src/ggml-metal/ggml-metal-embed.metal"
            ],
            sources: [
                "ggml/src/ggml.c",
                "src/whisper.cpp",
                "ggml/src/ggml-aarch64.c",
                "ggml/src/ggml-alloc.c",
                "ggml/src/ggml-backend.cpp",
                "ggml/src/ggml-backend-reg.cpp",
                "ggml/src/ggml-cpu/ggml-cpu.c",
                "ggml/src/ggml-cpu/ggml-cpu.cpp",
                "ggml/src/ggml-cpu/ggml-cpu-aarch64.c",
                "ggml/src/ggml-cpu/ggml-cpu-quants.c",
                "ggml/src/ggml-quants.c",
                "ggml/src/ggml-threading.cpp",
                "ggml/src/ggml-metal/ggml-metal.m"
            ],
            resources: [.process("ggml/src/ggml-metal/ggml-metal.metal")],
            publicHeadersPath: "spm-headers",
            cSettings: [
                .unsafeFlags(["-Wno-shorten-64-to-32", "-O3", "-DNDEBUG"]),
                .unsafeFlags(["-fno-objc-arc"]),
                .headerSearchPath("ggml/src"),
                .define("GGML_USE_ACCELERATE"),
                .define("GGML_USE_METAL")
                // NOTE: NEW_LAPACK will required iOS version 16.4+
                // We should consider add this in the future when we drop support for iOS 14
                // (ref: ref: https://developer.apple.com/documentation/accelerate/1513264-cblas_sgemm?language=objc)
                // .define("ACCELERATE_NEW_LAPACK"),
                // .define("ACCELERATE_LAPACK_ILP64")
            ],
            linkerSettings: [
                .linkedFramework("Accelerate")
            ]
        )
    ],
    cxxLanguageStandard: .cxx11
)
