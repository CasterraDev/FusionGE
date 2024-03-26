#include "renderer/vulkan/shaders/vulkanShadersUtil.h"
#include "core/fmemory.h"
#include "core/logger.h"
#include "platform/filesystem.h"
#include "core/fstring.h"
#include "resources/resourceManager.h"

b8 createShaderModule(vulkanHeader* header, char* name, char* shaderType, VkShaderStageFlagBits stageFlags, u32 stageIdx, vulkanShaderStage* outShaderStages){
    char filename[512];
    strFmt(filename, "shaders/%s.%s.spv", name, shaderType);

    resource binRes;
    if (!resourceLoad(filename, RESOURCE_TYPE_BINARY, &binRes)){
        FERROR("Unable to read shader file: %s", filename);
        return false;
    }

    fzeroMemory(&outShaderStages[stageIdx].moduleCreateInfo, sizeof(VkShaderModuleCreateInfo));
    outShaderStages[stageIdx].moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    outShaderStages[stageIdx].moduleCreateInfo.codeSize = binRes.dataSize;
    outShaderStages[stageIdx].moduleCreateInfo.pCode = (u32*)binRes.data;

    VULKANSUCCESS(vkCreateShaderModule(header->device.logicalDevice,
        &outShaderStages[stageIdx].moduleCreateInfo, header->allocator, &outShaderStages[stageIdx].module))

    resourceUnload(&binRes);

    fzeroMemory(&outShaderStages[stageIdx].stageCreateInfo, sizeof(VkPipelineShaderStageCreateInfo));
    outShaderStages[stageIdx].stageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    outShaderStages[stageIdx].stageCreateInfo.stage = stageFlags;
    outShaderStages[stageIdx].stageCreateInfo.module = outShaderStages[stageIdx].module;
    outShaderStages[stageIdx].stageCreateInfo.pName = "main";

    return true;
}