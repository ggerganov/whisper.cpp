require "mkmf"
require "tsort"

# TODO: options such as CoreML

cmake = find_executable("cmake") || abort

have_library("gomp") rescue nil

prefix = File.join("build", "whisper.cpp.dot")
system cmake, "-S", "sources", "-B", "build", "--graphviz", prefix, "-D", "BUILD_SHARED_LIBS=OFF", exception: true

static_lib_shape = nil
nodes = {}
depends = {}
class << depends
  include TSort
  alias tsort_each_node each_key
  def tsort_each_child(node, &block)
    fetch(node, []).each(&block)
  end
end
File.open(File.join("build", "whisper.cpp.dot")).each_line do |line|
  case line
  when /\[\s*label\s*=\s*"Static Library"\s*,\s*shape\s*=\s*(?<shape>\w+)\s*\]/
    static_lib_shape = $~[:shape]
  when /\A\s*"(?<node>\w+)"\s*\[\s*label\s*=\s*"(?<label>\S+)"\s*,\s*shape\s*=\s*(?<shape>\w+)\s*\]\s*;\s*\z/
    node = $~[:node]
    label = $~[:label]
    shape = $~[:shape]
    nodes[node] = [label, shape]
  when /\A\s*"(?<depender>\w+)"\s*->\s*"(?<dependee>\w+)"/
    depender = $~[:depender]
    dependee = $~[:dependee]
    depends[depender] ||= []
    depends[depender] << dependee
  end
end
libs = depends.tsort.filter_map {|node|
  label, shape = nodes[node]
  shape == static_lib_shape ? label : nil
}.collect {|lib| "lib#{lib}.a"}
 .reverse
 .join(" ")

$CFLAGS << " -std=c11 -fPIC"
$CXXFLAGS << " -std=c++17 -O3 -DNDEBUG"
$INCFLAGS << " -Isources/include -Isources/ggml/include -Isources/examples"
$LOCAL_LIBS << " #{libs}"
$cleanfiles << " build #{libs}"

create_makefile "whisper" do |conf|
  conf << <<~EOF
    $(TARGET_SO): #{libs}
    #{libs}: cmake-targets
    cmake-targets:
    #{"\t"}#{cmake} -S sources -B build -D BUILD_SHARED_LIBS=OFF -D CMAKE_ARCHIVE_OUTPUT_DIRECTORY=#{__dir__} -D CMAKE_POSITION_INDEPENDENT_CODE=ON
    #{"\t"}#{cmake} --build build --config Release --target common whisper
    #{"\t"}
  EOF
end
