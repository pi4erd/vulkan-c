GLSLC = glslc
GLSLANGVALIDATOR = glslangValidator
EMBEDDER = $(BINDIR)/embedder

CFLAGS = --target-env vulkan1.2 -V

SHADERS = main.vert main.frag ray.rgen ray.rmiss ray.rchit
SPV_FILES = $(SHADERS:%=$(SHADERBIN)/%.spv)
HEADER_FILES = $(SHADERS:%=$(RESINCLUDE)/%.h)

.PHONY: all clean

all: $(HEADER_FILES)

$(SPV_FILES): $(SHADERBIN)/%.spv: %
	@echo "Compiling $<"
	@$(GLSLANGVALIDATOR) $(CFLAGS) $< -o $@

$(HEADER_FILES): $(RESINCLUDE)/%.h: $(SHADERBIN)/%.spv
	@echo "Embedding $<"
	@$(EMBEDDER) -o $@ $<

clean:
	rm -f $(SPV_FILES) $(HEADER_FILES)