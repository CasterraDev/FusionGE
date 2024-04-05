#pragma once

#include "defines.h"
#include "renderer/vulkan/vulkanHeader.h"

typedef enum ShaderType{
    SHADERTYPE_VERT,
    SHADERTYPE_FRAG
} ShaderType;

// https://vulkan-tutorial.com/Drawing_a_triangle/Graphics_pipeline_basics/Shader_modules

b8 createShaderModule(vulkanHeader* header, char* name, char* shaderType, VkShaderStageFlagBits stageFlags, u32 stageIdx, vulkanShaderStage* outShaderStages);
