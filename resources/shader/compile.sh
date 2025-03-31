#!/bin/sh

GLSLC=glslc

$GLSLC main.vert -o $SHADERBIN/main.vert.spv
$GLSLC main.frag -o $SHADERBIN/main.frag.spv

$BINDIR/embedder $SHADERBIN/main.vert.spv -o main.vert.h
$BINDIR/embedder $SHADERBIN/main.frag.spv -o main.frag.h

mv main.vert.h $RESINCLUDE/main.vert.h
mv main.frag.h $RESINCLUDE/main.frag.h
