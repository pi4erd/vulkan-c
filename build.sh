#!/bin/sh

CC=cc
LD=gcc
CFLAGS="-std=c17 -Wall -Wextra -Wpedantic"
LDFLAGS="-lc -lvulkan -lglfw"

BUILDDIR=build
SRCDIR=src
TARGET=$BUILDDIR/vulkan-c

FILES="$SRCDIR/main.c $SRCDIR/engine.c $SRCDIR/array.c $SRCDIR/device_api.c $SRCDIR/window.c"

FILES=(main.c engine.c array.c device_api.c device_utils.c window.c swapchain.c)
OBJFILES=${FILES[@]/#/$BUILDDIR\/}
OBJFILES=${OBJFILES//.c/.o}

echo "C flags: $CFLAGS"
echo "Linker flags: $LDFLAGS"
echo "Files to build: ${FILES[@]}"

mkdir -p $BUILDDIR

for file in ${FILES[@]}
do
    objfile=${file%.*}.o
    COMMAND="$CC $CFLAGS -c -o $BUILDDIR/$objfile $SRCDIR/$file"
    echo "$COMMAND"
    $COMMAND
done

echo ${FILES[@]}
echo $OBJFILES
COMMAND="$LD $LDFLAGS -o $TARGET $OBJFILES"
echo $COMMAND
$COMMAND
