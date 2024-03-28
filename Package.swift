// swift-tools-version:5.5

import PackageDescription

var sources = [
  "ggml.c",
  "whisper.cpp",
  "ggml-alloc.c",
  "ggml-backend.c",
  "ggml-quants.c",
]
var resources: [Resource] = []

var cSettings: [CSetting] = [
  .unsafeFlags(["-Wno-shorten-64-to-32", "-O3", "-DNDEBUG"]),
  .unsafeFlags(["-fno-objc-arc"]),
]

var linkerSettings: [LinkerSetting] = [.linkedFramework("Accelerate")]

#if os(Linux)
  // .define("_GNU_SOURCE"),
  cSettings.append(.define("WHISPER_NO_ACCELERATE"))
  cSettings.append(.define("_GNU_SOURCE"))

#else
  resources.append(
    .process("ggml-metal.metal")
  )
  // Adding metal source if the target is an Apple platform.
  sources.append("ggml-metal.m")

  cSettings.append([
    .define("GGML_USE_ACCELERATE"),
    .define("GGML_USE_METAL"),
    // NOTE: NEW_LAPACK will required iOS version 16.4+
    // We should consider add this in the future when we drop support for iOS 14
    // (ref: ref: https://developer.apple.com/documentation/accelerate/1513264-cblas_sgemm?language=objc)
    // .define("ACCELERATE_NEW_LAPACK"),
    // .define("ACCELERATE_LAPACK_ILP64")
  ])

#endif

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
            sources: sources,
            resources: resources,
            publicHeadersPath: "spm-headers",
            cSettings: cSettings,
            linkerSettings: linkerSettings
        )
    ],
    cxxLanguageStandard: .cxx11
)
