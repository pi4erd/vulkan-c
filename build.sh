#!/bin/sh

export CC="cc"
export LD="gcc"
export CFLAGS="-std=c17 -Wall -Wextra -Wpedantic"
LDFLAGS="-lc -lvulkan -lglfw"

export BUILDDIR=$(pwd)/build
export BINDIR=$BUILDDIR/bin
export OBJDIR=$BUILDDIR/obj
export SRCDIR=$(pwd)/src

TARGET=$BUILDDIR/bin/vulkan-c

FILES=(
    main.c engine.c array.c device_api.c 
    device_utils.c window.c swapchain.c app.c
)

OBJFILES=${FILES[@]/#/$OBJDIR\/}
OBJFILES=${OBJFILES//.c/.o}

echo "C flags: $CFLAGS"
echo "Linker flags: $LDFLAGS"
echo "Files to build: ${FILES[@]}"

# Build auxilary projects
(cd embedder; ./build.sh)

# Build main project
mkdir -p $BUILDDIR $BINDIR $OBJDIR

for file in ${FILES[@]}
do
    objfile=${file%.*}.o
    COMMAND="$CC $CFLAGS -c -o $OBJDIR/$objfile $SRCDIR/$file"
    echo "$COMMAND"
    $COMMAND
done

echo ${FILES[@]}
echo $OBJFILES
COMMAND="$LD $LDFLAGS -o $TARGET $OBJFILES"
echo $COMMAND
$COMMAND

