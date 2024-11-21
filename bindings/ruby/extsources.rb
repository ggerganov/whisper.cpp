EXTSOURCES = `git ls-files -z ../..`.split("\x0").grep(%r{\A\.\./\.\./(?:src|include|ggml).+\.(c|cpp|h|m|metal)\z}) <<
             "../../examples/dr_wav.h" <<
             "../../scripts/get-flags.mk" <<
             "../../LICENSE"
