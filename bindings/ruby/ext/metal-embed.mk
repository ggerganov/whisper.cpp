ggml/src/ggml-metal/ggml-metal-embed.o: \
	ggml/src/ggml-metal/ggml-metal.metal \
	ggml/src/ggml-metal/ggml-metal-impl.h \
	ggml/src/ggml-common.h
	@echo "Embedding Metal library"
	@sed -e '/__embed_ggml-common.h__/r      ggml/src/ggml-common.h'                -e '/__embed_ggml-common.h__/d'      < ggml/src/ggml-metal/ggml-metal.metal           > ggml/src/ggml-metal/ggml-metal-embed.metal.tmp
	@sed -e '/#include "ggml-metal-impl.h"/r ggml/src/ggml-metal/ggml-metal-impl.h' -e '/#include "ggml-metal-impl.h"/d' < ggml/src/ggml-metal/ggml-metal-embed.metal.tmp > ggml/src/ggml-metal/ggml-metal-embed.metal
	$(eval TEMP_ASSEMBLY=$(shell mktemp -d))
	@echo ".section __DATA, __ggml_metallib"                       >  $(TEMP_ASSEMBLY)/ggml-metal-embed.s
	@echo ".globl _ggml_metallib_start"                            >> $(TEMP_ASSEMBLY)/ggml-metal-embed.s
	@echo "_ggml_metallib_start:"                                  >> $(TEMP_ASSEMBLY)/ggml-metal-embed.s
	@echo ".incbin \"ggml/src/ggml-metal/ggml-metal-embed.metal\"" >> $(TEMP_ASSEMBLY)/ggml-metal-embed.s
	@echo ".globl _ggml_metallib_end"                              >> $(TEMP_ASSEMBLY)/ggml-metal-embed.s
	@echo "_ggml_metallib_end:"                                    >> $(TEMP_ASSEMBLY)/ggml-metal-embed.s
	$(CC) $(CFLAGS) -c $(TEMP_ASSEMBLY)/ggml-metal-embed.s -o $@
	@rm -f ${TEMP_ASSEMBLY}/ggml-metal-embed.s
	@rmdir ${TEMP_ASSEMBLY}
