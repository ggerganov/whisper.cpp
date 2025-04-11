require "mkmf"

# TODO: options such as CoreML

cmake = find_executable("cmake") || abort

prefix = File.join("build", "whisper.cpp.dot")
system cmake, "-S", "sources", "-B", "build", "--graphviz", prefix, "-D", "BUILD_SHARED_LIBS=OFF", exception: true
libs = Dir["#{prefix}.*"].collect {|file|
  "lib" + file.sub("#{prefix}.", "") + ".a"
}

$INCFLAGS << " -Isources/include -Isources/ggml/include -Isources/examples"
$LOCAL_LIBS << " " << libs.join(" ")
$cleanfiles << " build" << libs.join(" ")

create_makefile "whisper" do |conf|
  conf << <<~EOF
    $(TARGET_SO): #{libs.join(" ")}
    #{libs.join(" ")}: cmake-targets
    cmake-targets:
    #{"\t"}#{cmake} -S sources -B build -D BUILD_SHARED_LIBS=OFF -D CMAKE_ARCHIVE_OUTPUT_DIRECTORY=#{__dir__}
    #{"\t"}#{cmake} --build build --config Release --target common whisper
  EOF
end
