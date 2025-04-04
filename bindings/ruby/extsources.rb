require "yaml"

sources = `git ls-files -z ../.. --recurse-submodules`.split("\x0")
paths = YAML.load_file("../../.github/workflows/bindings-ruby.yml")[true]["push"]["paths"]
paths.delete "bindings/ruby/**"
EXTSOURCES = (Dir.glob(paths, base: "../..").collect {|path| "../../#{path}"} << "../../LICENSE") & sources
