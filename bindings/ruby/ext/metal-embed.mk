ggml-metal-embed.o: \
	ggml-metal.metal \
	ggml-common.h
	@echo "Embedding Metal library"
	@sed -e '/#include "ggml-common.h"/r ggml-common.h' -e '/#include "ggml-common.h"/d' < ggml-metal.metal > ggml-metal-embed.metal
	$(eval TEMP_ASSEMBLY=$(shell mktemp))
	@echo ".section __DATA, __ggml_metallib"            >  $(TEMP_ASSEMBLY)
	@echo ".globl _ggml_metallib_start"                 >> $(TEMP_ASSEMBLY)
	@echo "_ggml_metallib_start:"                       >> $(TEMP_ASSEMBLY)
	@echo ".incbin \"ggml-metal-embed.metal\""          >> $(TEMP_ASSEMBLY)
	@echo ".globl _ggml_metallib_end"                   >> $(TEMP_ASSEMBLY)
	@echo "_ggml_metallib_end:"                         >> $(TEMP_ASSEMBLY)
	@$(AS) $(TEMP_ASSEMBLY) -o $@
	@rm -f ${TEMP_ASSEMBLY}
