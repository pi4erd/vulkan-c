#!/bin/sh

echo "Building embedder."

LDFLAGS=

TARGET=$BINDIR/embedder

COMMAND="$CC $CFLAGS $LDFLAGS -o $TARGET embedder.c"
echo $COMMAND
$COMMAND
