ggml/src/ggml-metal/ggml-metal.o: \
	ggml/src/ggml-metal/ggml-metal.m \
	ggml/src/ggml-metal/ggml-metal-impl.h \
	ggml/include/ggml-metal.h \
	ggml/include/ggml.h
	$(CC) $(CFLAGS) -c $< -o $@
