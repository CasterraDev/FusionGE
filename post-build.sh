#!/bin/bash

# Run from root directory!
mkdir -p Assets
mkdir -p Assets/shaders

echo "Compiling shaders..."

echo "assets/shaders/Builtin.ObjectShader.vert.glsl -> Assets/shaders/Builtin.ObjectShader.vert.spv"
$VULKAN_SDK/bin/glslc -fshader-stage=vert assets/shaders/Builtin.ObjectShader.vert.glsl -o Assets/shaders/Builtin.ObjectShader.vert.spv
ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
echo "Error:"$ERRORLEVEL && exit
fi

echo "assets/shaders/Builtin.ObjectShader.frag.glsl -> Assets/shaders/Builtin.ObjectShader.frag.spv"
$VULKAN_SDK/bin/glslc -fshader-stage=frag assets/shaders/Builtin.ObjectShader.frag.glsl -o Assets/shaders/Builtin.ObjectShader.frag.spv
ERRORLEVEL=$?
if [ $ERRORLEVEL -ne 0 ]
then
echo "Error:"$ERRORLEVEL && exit
fi

echo "Done."