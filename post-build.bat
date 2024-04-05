@echo off

REM Run from root directory!
if not exist "%cd%\Assets\shaders\" mkdir "%cd%\Assets\shaders"

echo "Compiling shaders..."

echo "assets/shaders/App.MaterialShader.vert.glsl -> Assets/shaders/App.MaterialShader.vert.spv"
%VULKAN_SDK%\bin\glslc.exe -fshader-stage=vert assets/shaders/App.MaterialShader.vert.glsl -o Assets/shaders/App.MaterialShader.vert.spv
IF %ERRORLEVEL% NEQ 0 (echo Error: %ERRORLEVEL% && exit)

echo "assets/shaders/App.MaterialShader.frag.glsl -> Assets/shaders/App.MaterialShader.frag.spv"
%VULKAN_SDK%\bin\glslc.exe -fshader-stage=frag assets/shaders/App.MaterialShader.frag.glsl -o Assets/shaders/App.MaterialShader.frag.spv
IF %ERRORLEVEL% NEQ 0 (echo Error: %ERRORLEVEL% && exit)

echo "assets/shaders/App.UIShader.vert.glsl -> Assets/shaders/App.UIShader.vert.spv"
%VULKAN_SDK%\bin\glslc.exe -fshader-stage=vert assets/shaders/App.UIShader.vert.glsl -o Assets/shaders/App.UIShader.vert.spv
IF %ERRORLEVEL% NEQ 0 (echo Error: %ERRORLEVEL% && exit)

echo "assets/shaders/App.UIShader.frag.glsl -> Assets/shaders/App.UIShader.frag.spv"
%VULKAN_SDK%\bin\glslc.exe -fshader-stage=frag assets/shaders/App.UIShader.frag.glsl -o Assets/shaders/App.UIShader.frag.spv
IF %ERRORLEVEL% NEQ 0 (echo Error: %ERRORLEVEL% && exit)

echo "Done." 