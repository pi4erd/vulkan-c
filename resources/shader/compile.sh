#!/bin/sh

GLSLC=glslc
GLSLANGVALIDATOR=glslangValidator
echo "Compiling shaders."

$GLSLC main.vert -o $SHADERBIN/main.vert.spv
$GLSLC main.frag -o $SHADERBIN/main.frag.spv

# Ray tracing
$GLSLANGVALIDATOR --target-env vulkan1.2 -V ray.rgen -o $SHADERBIN/ray.rgen.spv
$GLSLANGVALIDATOR --target-env vulkan1.2 -V ray.rmiss -o $SHADERBIN/ray.rmiss.spv
$GLSLANGVALIDATOR --target-env vulkan1.2 -V ray.rchit -o $SHADERBIN/ray.rchit.spv

$BINDIR/embedder $SHADERBIN/main.vert.spv -o main.vert.h
$BINDIR/embedder $SHADERBIN/main.frag.spv -o main.frag.h

# Ray tracing
$BINDIR/embedder $SHADERBIN/ray.rgen.spv -o ray.rgen.h
$BINDIR/embedder $SHADERBIN/ray.rmiss.spv -o ray.rmiss.h
$BINDIR/embedder $SHADERBIN/ray.rchit.spv -o ray.rchit.h

mv main.vert.h $RESINCLUDE/main.vert.h
mv main.frag.h $RESINCLUDE/main.frag.h

# Ray tracing
mv ray.rgen.h $RESINCLUDE/ray.rgen.h
mv ray.rmiss.h $RESINCLUDE/ray.rmiss.h
mv ray.rchit.h $RESINCLUDE/ray.rchit.h
