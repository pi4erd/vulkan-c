# Config
CC := cc
LD := gcc

# Global dirs
BUILDDIR := $(shell pwd)/build
RESOURCEDIR := $(shell pwd)/resources
SRCDIR := $(shell pwd)/src

# Relative dirs
BINDIR := $(BUILDDIR)/bin
OBJDIR := $(BUILDDIR)/obj
SHADERBIN := $(OBJDIR)/shader
RESINCLUDE := $(RESOURCEDIR)/include
RESSHADER := $(RESOURCEDIR)/shader

export

# Create necessary directories
$(shell mkdir -p $(BUILDDIR) $(BINDIR) $(OBJDIR) $(SHADERBIN))

# Main project settings
CFLAGS := -std=c17 -I$(RESINCLUDE) -Wall -Wextra -Wpedantic \
          $(shell pkg-config --cflags glfw3) \
		  $(shell pkg-config --cflags vulkan)
LDFLAGS := -lc $(shell pkg-config --libs glfw3) $(shell pkg-config --libs vulkan)

ifdef MACOS
    CFLAGS += -I/opt/homebrew/include -I/usr/local/include
    LDFLAGS += -L/opt/homebrew/lib -rpath /usr/local/lib
endif

ifdef RELEASE
    CFLAGS += -DRELEASE -O2 -funroll-loops -Werror
else
    CFLAGS += -ggdb
endif

# Files and targets
TARGET := $(BINDIR)/vulkan-c
FILES := main.c engine.c device_api.c vkalloc.c mesh.c \
         device_utils.c window.c swapchain.c app.c
HEADERS := $(wildcard $(SRCDIR)/*.h)
OBJFILES := $(addprefix $(OBJDIR)/, $(FILES:.c=.o))

# Default target
all: $(TARGET)

# Build auxiliary projects
.PHONY: embedder shaders
embedder:
	$(MAKE) -C embedder

shaders: embedder
	$(MAKE) -C $(RESSHADER)

# Build main target
$(TARGET): $(OBJFILES)
	$(LD) $(LDFLAGS) -o $(TARGET) $(OBJFILES)

# Compile source files into object files
$(OBJDIR)/%.o: $(SRCDIR)/%.c $(HEADERS) shaders
	$(CC) $(CFLAGS) -c -o $@ $<

# Clean build artifacts
.PHONY: clean
clean:
	$(MAKE) -C $(RESSHADER) clean
	rm -rf $(BUILDDIR)
