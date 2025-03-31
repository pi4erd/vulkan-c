#!/bin/sh

echo "Building embedder."

CFLAGS="-std=c17 -I$RESINCLUDE -Wall -Wextra -Wpedantic"
LDFLAGS=

TARGET=$BINDIR/embedder

COMMAND="$CC $CFLAGS $LDFLAGS -o $TARGET embedder.c"
echo $COMMAND
$COMMAND
