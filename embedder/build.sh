#!/bin/sh

echo "Building embedder."

CFLAGS="-std=c17 -Wall -Wextra -Wpedantic"
LDFLAGS=

TARGET=$BINDIR/embedder

COMMAND="$CC $CFLAGS $LDFLAGS -o $TARGET embedder.c"
echo $COMMAND
$COMMAND
