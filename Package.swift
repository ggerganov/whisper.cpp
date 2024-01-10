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
    dependencies: [
        .package(url: "https://github.com/ggerganov/ggml.git", .revision("8bf3f009e653f6bdac893c4bb6441f88ee55fe48"))
    ],
    targets: [
        .target(
            name: "whisper",
            dependencies: ["ggml"],
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
               "Makefile"
            ],
            sources: [
                "whisper.cpp",
            ],
            publicHeadersPath: "spm-headers",
            cSettings: [
                .unsafeFlags(["-Wno-shorten-64-to-32", "-O3", "-DNDEBUG"]),
                .define("GGML_USE_ACCELERATE"),
                .unsafeFlags(["-fno-objc-arc"]),
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
