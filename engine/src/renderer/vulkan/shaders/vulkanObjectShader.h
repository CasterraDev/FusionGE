#pragma once

#include "defines.h"
#include "renderer/vulkan/vulkanHeader.h"

b8 vulkanShaderCreate(vulkanHeader* header, vulkanShader* outShader);

void vulkanShaderDestroy(vulkanHeader* header, vulkanShader* shader);

void vulkanShaderUse(vulkanHeader* header, vulkanShader* shader);

void vulkanShaderUpdateGlobalState(vulkanHeader* header, vulkanShader* shader);

void vulkanShaderSetModel(vulkanHeader* header, struct vulkanShader* shader, mat4 model);
void vulkanShaderApplyMaterial(vulkanHeader* header, struct vulkanShader* shader, material* material);

b8 vulkanShaderResourceAcquire(vulkanHeader* header, vulkanShader* shader, material* mat);
void vulkanShaderResourceRelease(vulkanHeader* header, vulkanShader* shader, material* mat);