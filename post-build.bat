@echo off

REM Run from root directory!
if not exist "%cd%\Assets\shaders\" mkdir "%cd%\Assets\shaders"

echo "Compiling shaders..."

echo "assets/shaders/App.ObjectShader.vert.glsl -> Assets/shaders/App.ObjectShader.vert.spv"
%VULKAN_SDK%\bin\glslc.exe -fshader-stage=vert assets/shaders/App.ObjectShader.vert.glsl -o Assets/shaders/App.ObjectShader.vert.spv
IF %ERRORLEVEL% NEQ 0 (echo Error: %ERRORLEVEL% && exit)

echo "assets/shaders/App.ObjectShader.frag.glsl -> Assets/shaders/App.ObjectShader.frag.spv"
%VULKAN_SDK%\bin\glslc.exe -fshader-stage=frag assets/shaders/App.ObjectShader.frag.glsl -o Assets/shaders/App.ObjectShader.frag.spv
IF %ERRORLEVEL% NEQ 0 (echo Error: %ERRORLEVEL% && exit)

echo "Done." 