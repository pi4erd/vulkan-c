#!/bin/sh

# Config
export CC="cc"
export LD="gcc"

# Global dirs
export BUILDDIR=$(pwd)/build
export RESOURCEDIR=$(pwd)/resources
export SRCDIR=$(pwd)/src

# Relative dirs
export BINDIR=$BUILDDIR/bin
export OBJDIR=$BUILDDIR/obj
export SHADERBIN=$OBJDIR/shader
export RESINCLUDE=$RESOURCEDIR/include
export RESSHADER=$RESOURCEDIR/shader

mkdir -p $BUILDDIR $BINDIR $OBJDIR $SHADERBIN

# Main project settings
CFLAGS="-std=c17 -I$RESINCLUDE -Wall -Wextra -Wpedantic"
LDFLAGS="-lc -lvulkan -lglfw"

if [[  x"${MACOS}" != "x" ]]; then
    CFLAGS+=" -I/opt/homebrew/include -I/usr/local/include"
    LDFLAGS+=" -L/opt/homebrew/lib -rpath /usr/local/lib"
fi

if [[ x"${RELEASE}" != "x" ]]; then
    # Release mode
    CFLAGS+=" -O2 -funroll-loops -Werror"
else
    # Debug mode
    CFLAGS+=" -ggdb"
fi

# Build auxilary projects
(cd embedder; ./build.sh)
(cd $RESSHADER; ./compile.sh)

# Build main project

TARGET=$BUILDDIR/bin/vulkan-c

FILES=(
    main.c engine.c device_api.c vkalloc.c mesh.c
    device_utils.c window.c swapchain.c app.c
)

OBJFILES=${FILES[@]/#/$OBJDIR\/}
OBJFILES=${OBJFILES//.c/.o}

echo "C flags: $CFLAGS"
echo "Linker flags: $LDFLAGS"
echo "Files to build: ${FILES[@]}"

for file in ${FILES[@]}
do
    objfile=${file%.*}.o
    COMMAND="$CC $CFLAGS -c -o $OBJDIR/$objfile $SRCDIR/$file"
    echo "$COMMAND"
    $COMMAND
done

COMMAND="$LD $LDFLAGS -o $TARGET $OBJFILES"
echo $COMMAND
$COMMAND
