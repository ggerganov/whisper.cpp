ggml/src/ggml-cpu/ggml-cpu-cpp.o: \
	ggml/src/ggml-cpu/ggml-cpu.cpp \
	ggml/include/ggml-backend.h \
	ggml/include/ggml.h \
	ggml/include/ggml-alloc.h \
	ggml/src/ggml-backend-impl.h \
	ggml/include/ggml-cpu.h \
	ggml/src/ggml-impl.h
	$(CXX) $(CXXFLAGS)   -c $< -o $@
