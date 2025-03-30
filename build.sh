#!/bin/sh

CC=cc
CFLAGS="-std=c11 -Wall -Wextra -Wpedantic"
LDFLAGS="-lvulkan -lglfw"

BUILDDIR=build
SRCDIR=src
TARGET=$BUILDDIR/vulkan-c

FILES="$SRCDIR/main.c $SRCDIR/engine.c $SRCDIR/array.c $SRCDIR/device_api.c $SRCDIR/window.c"

COMMAND="$CC $CFLAGS $LDFLAGS -o $TARGET $FILES"

echo "C flags: $CFLAGS"
echo "Linker flags: $LDFLAGS"
echo "Files to build: $FILES"

mkdir -p $BUILDDIR

echo "$COMMAND"
$COMMAND
