#pragma once

#include "defines.h"
#include "renderer/vulkan/vulkanHeader.h"

b8 vulkanMaterialShaderCreate(vulkanHeader* header,
                              vulkanOverallShader* outShader);

void vulkanMaterialShaderDestroy(vulkanHeader* header,
                                 vulkanOverallShader* shader);

void vulkanMaterialShaderUse(vulkanHeader* header, vulkanOverallShader* shader);

void vulkanMaterialShaderUpdateGlobalState(vulkanHeader* header,
                                           vulkanOverallShader* shader);

void vulkanMaterialShaderSetModel(vulkanHeader* header,
                                  struct vulkanOverallShader* shader,
                                  mat4 model);
void vulkanMaterialShaderApplyMaterial(vulkanHeader* header,
                                       struct vulkanOverallShader* shader,
                                       material* material);

b8 vulkanMaterialShaderResourceAcquire(vulkanHeader* header,
                                       vulkanOverallShader* shader,
                                       material* mat);
void vulkanMaterialShaderResourceRelease(vulkanHeader* header,
                                         vulkanOverallShader* shader,
                                         material* mat);
