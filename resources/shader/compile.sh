#!/bin/sh

GLSLC=glslc

echo "Compiling shaders."

$GLSLC main.vert -o $SHADERBIN/main.vert.spv
$GLSLC main.frag -o $SHADERBIN/main.frag.spv
$GLSLC ray.rgen -o $SHADERBIN/ray.rgen.spv

$BINDIR/embedder $SHADERBIN/main.vert.spv -o main.vert.h
$BINDIR/embedder $SHADERBIN/main.frag.spv -o main.frag.h
$BINDIR/embedder $SHADERBIN/ray.rgen.spv -o ray.rgen.h

mv main.vert.h $RESINCLUDE/main.vert.h
mv main.frag.h $RESINCLUDE/main.frag.h
mv ray.rgen.h $RESINCLUDE/ray.rgen.h
