#pragma once

#include "defines.h"
#include "renderer/vulkan/vulkanHeader.h"

b8 vulkanUIShaderCreate(vulkanHeader* header, vulkanOverallShader* outShader);

void vulkanUIShaderDestroy(vulkanHeader* header, vulkanOverallShader* shader);

void vulkanUIShaderUse(vulkanHeader* header, vulkanOverallShader* shader);

void vulkanUIShaderUpdateGlobalState(vulkanHeader* header, vulkanOverallShader* shader);

void vulkanUIShaderSetModel(vulkanHeader* header, struct vulkanOverallShader* shader, mat4 model);
void vulkanUIShaderApplyMaterial(vulkanHeader* header, struct vulkanOverallShader* shader, material* material);

b8 vulkanUIShaderResourceAcquire(vulkanHeader* header, vulkanOverallShader* shader, material* mat);
void vulkanUIShaderResourceRelease(vulkanHeader* header, vulkanOverallShader* shader, material* mat);