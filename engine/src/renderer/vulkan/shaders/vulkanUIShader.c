#include "vulkanUIShader.h"
#include "core/fmemory.h"
#include "core/fstring.h"
#include "core/logger.h"
#include "math/fsnmath.h"
#include "math/matrixMath.h"
#include "renderer/vulkan/vulkanBuffers.h"
#include "renderer/vulkan/vulkanPipeline.h"
#include "resources/resourcesTypes.h"
#include "systems/textureSystem.h"
#include "vulkanShadersUtil.h"

#define APPLICATION_SHADER_PREFIX "App.UIShader"

b8 vulkanUIShaderCreate(vulkanHeader* header, vulkanOverallShader* outShader) {
    // Shader module init per stage.
    char stage_type_strs[MATERIAL_SHADER_STAGE_COUNT][5] = {"vert", "frag"};
    VkShaderStageFlagBits stage_types[MATERIAL_SHADER_STAGE_COUNT] = {
        VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};
    for (u32 i = 0; i < MATERIAL_SHADER_STAGE_COUNT; ++i) {
        if (!createShaderModule(header, APPLICATION_SHADER_PREFIX,
                                stage_type_strs[i], stage_types[i], i,
                                outShader->stages)) {
            FERROR("Failed to create %s shader: %s", stage_type_strs[i],
                   APPLICATION_SHADER_PREFIX);
            return false;
        }
    }

    // Create Global Descriptor Set Layout
    VkDescriptorSetLayoutBinding uboLayoutBinding;
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = 0;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;
    VULKANSUCCESS(vkCreateDescriptorSetLayout(
        header->device.logicalDevice, &layoutInfo, header->allocator,
        &outShader->globalDescriptorSetLayout));

    VkDescriptorPoolSize poolSize;
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = header->swapchain.imageCnt;

    VkDescriptorPoolCreateInfo globalPoolInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    globalPoolInfo.poolSizeCount = 1;
    globalPoolInfo.pPoolSizes = &poolSize;
    globalPoolInfo.maxSets = header->swapchain.imageCnt;

    outShader->samplerTypes[0] = TEXTURE_USE_MAP_DIFFUSE;

    if (vkCreateDescriptorPool(
            header->device.logicalDevice, &globalPoolInfo, header->allocator,
            &outShader->globalDescriptorPool) != VK_SUCCESS) {
        FERROR("Failed to create descriptor pool.")
        return false;
    }

    // Object stuff
    const u32 localSamplerCnt = 1;
    VkDescriptorType descriptorTypes[VULKAN_OVERALL_SHADER_DESCRIPTOR_COUNT] = {
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         // Binding 0 - uniform buffer
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // Binding 1 - Diffuse
                                                   // sampler layout.
    };

    VkDescriptorSetLayoutBinding
        bindings[VULKAN_OVERALL_SHADER_DESCRIPTOR_COUNT];
    fzeroMemory(&bindings, sizeof(VkDescriptorSetLayoutBinding) *
                               VULKAN_OVERALL_SHADER_DESCRIPTOR_COUNT);
    for (u32 i = 0; i < VULKAN_OVERALL_SHADER_DESCRIPTOR_COUNT; ++i) {
        bindings[i].binding = i;
        bindings[i].descriptorCount = 1;
        bindings[i].descriptorType = descriptorTypes[i];
        bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    VkDescriptorSetLayoutCreateInfo layout_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layout_info.bindingCount = VULKAN_OVERALL_SHADER_DESCRIPTOR_COUNT;
    layout_info.pBindings = bindings;
    VULKANSUCCESS(
        vkCreateDescriptorSetLayout(header->device.logicalDevice, &layout_info,
                                    0, &outShader->objectDescriptorSetLayout));

    // Local/Object descriptor pool: Used for object-specific items like diffuse
    // color
    VkDescriptorPoolSize objectPoolSizes[2];
    // The first section will be used for uniform buffers
    objectPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    objectPoolSizes[0].descriptorCount = VULKAN_OVERALL_MAX_OBJECT_COUNT;
    // The second section will be used for image samplers.
    objectPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    objectPoolSizes[1].descriptorCount =
        localSamplerCnt * VULKAN_OVERALL_MAX_OBJECT_COUNT;

    VkDescriptorPoolCreateInfo objectPoolInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    objectPoolInfo.poolSizeCount = 2;
    objectPoolInfo.pPoolSizes = objectPoolSizes;
    objectPoolInfo.maxSets = VULKAN_OVERALL_MAX_OBJECT_COUNT;
    objectPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    // Create object descriptor pool.
    VULKANSUCCESS(vkCreateDescriptorPool(header->device.logicalDevice,
                                         &objectPoolInfo, header->allocator,
                                         &outShader->objectDescriptorPool));

    // Pipeline creation
    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = (f32)header->framebufferHeight;
    viewport.width = (f32)header->framebufferWidth;
    viewport.height = -(f32)header->framebufferHeight;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // Scissor
    VkRect2D scissor;
    scissor.offset.x = scissor.offset.y = 0;
    scissor.extent.width = header->framebufferWidth;
    scissor.extent.height = header->framebufferHeight;

    // Attributes
    // TODO: Make this configurable
    u32 offset = 0;
#define ATTRIBUTE_COUNT 2
    VkVertexInputAttributeDescription attrDescs[ATTRIBUTE_COUNT];
    // NOTE: Must be in the same order as vertex3D
    // Position, Texcoord
    VkFormat formats[ATTRIBUTE_COUNT] = {VK_FORMAT_R32G32_SFLOAT,
                                         VK_FORMAT_R32G32_SFLOAT};
    u64 sizes[ATTRIBUTE_COUNT] = {sizeof(vector2), sizeof(vector2)};
    for (u32 i = 0; i < ATTRIBUTE_COUNT; ++i) {
        attrDescs[i].binding = 0;  // binding index - should match binding desc
        attrDescs[i].location = i; // attrib location
        attrDescs[i].format = formats[i];
        attrDescs[i].offset = offset;
        offset += sizes[i];
    }

#define DESCRIPTOR_SET_LAYOUTS_CNT 2
    VkDescriptorSetLayout descriptorSetLayouts[DESCRIPTOR_SET_LAYOUTS_CNT] = {
        outShader->globalDescriptorSetLayout,
        outShader->objectDescriptorSetLayout};

#define STAGE_CNT 2
    VkPipelineShaderStageCreateInfo stageCreateInfos[STAGE_CNT];
    fzeroMemory(stageCreateInfos, sizeof(stageCreateInfos));
    for (u32 i = 0; i < STAGE_CNT; ++i) {
        stageCreateInfos[i].sType = outShader->stages[i].stageCreateInfo.sType;
        stageCreateInfos[i] = outShader->stages[i].stageCreateInfo;
    }

    vulkanPipelineConfig config;
    config.name = "UI Graphics Pipeline";
    config.attrCnt = ATTRIBUTE_COUNT;
    config.attributes = attrDescs;
    config.renderpass = &header->uiRenderpass;
    config.scissor = scissor;
    config.viewport = viewport;
    config.stride = sizeof(vertex2D);
    config.stages = stageCreateInfos;
    config.stageCnt = STAGE_CNT;
    config.isWireframe = false;
    config.shouldDepthTest = false;
    config.descriptorSetLayoutCnt = DESCRIPTOR_SET_LAYOUTS_CNT;
    config.descriptorSetLayouts = descriptorSetLayouts;

    if (!vulkanPipelineCreate(header, &config, &outShader->pipeline)) {
        FERROR("Failed to create pipeline: %s", config.name);
    }

    VkDeviceSize bufferSize = sizeof(sceneUBO) * 3;
    // Create the uniform buffer
    vulkanBufferCreate(header, bufferSize,
                       VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                           VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       true, &outShader->globalUniformBuffer);

    // TODO: Make this configurable
    VkDescriptorSetLayout layouts[3] = {outShader->globalDescriptorSetLayout,
                                        outShader->globalDescriptorSetLayout,
                                        outShader->globalDescriptorSetLayout};

    VkDescriptorSetAllocateInfo allocInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocInfo.descriptorPool = outShader->globalDescriptorPool;
    allocInfo.descriptorSetCount = header->swapchain.imageCnt;
    allocInfo.pSetLayouts = layouts;

    if (vkAllocateDescriptorSets(header->device.logicalDevice, &allocInfo,
                                 outShader->globalDescriptorSets) !=
        VK_SUCCESS) {
        FERROR("Failed to allocate descriptor sets.")
        return false;
    }

    // Create the object uniform buffer.
    if (!vulkanBufferCreate(header, sizeof(materialUBO) * MAX_MATERIAL_COUNT,
                            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            // VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                            true, &outShader->objectUniformBuffer)) {
        FERROR("UI instance buffer creation failed for shader.");
        return false;
    }

    return true;
}

void vulkanUIShaderDestroy(vulkanHeader* header, vulkanOverallShader* shader) {
    vkDestroyDescriptorPool(header->device.logicalDevice,
                            shader->objectDescriptorPool, header->allocator);
    vkDestroyDescriptorSetLayout(header->device.logicalDevice,
                                 shader->objectDescriptorSetLayout,
                                 header->allocator);

    vulkanBufferDestroy(header, &shader->globalUniformBuffer);
    vulkanBufferDestroy(header, &shader->objectUniformBuffer);

    vulkanPipelineDestroy(header, &shader->pipeline);
    vkDestroyDescriptorPool(header->device.logicalDevice,
                            shader->globalDescriptorPool, header->allocator);
    vkDestroyDescriptorSetLayout(header->device.logicalDevice,
                                 shader->globalDescriptorSetLayout,
                                 header->allocator);

    for (u32 i = 0; i < 2; i++) {
        vkDestroyShaderModule(header->device.logicalDevice,
                              header->uiShader.stages[i].module,
                              header->allocator);
        header->uiShader.stages[i].module = 0;
    }
}

void vulkanUIShaderUse(vulkanHeader* header, vulkanOverallShader* shader) {
    vulkanPipelineBind(&header->graphicsCommandBuffers[header->imageIdx],
                       VK_PIPELINE_BIND_POINT_GRAPHICS, &shader->pipeline);
}

void vulkanUIShaderUpdateGlobalState(vulkanHeader* header,
                                     vulkanOverallShader* shader) {
    u32 imgIdx = header->imageIdx;
    VkCommandBuffer cb = header->graphicsCommandBuffers[imgIdx].handle;
    VkDescriptorSet globalDescriptor = shader->globalDescriptorSets[imgIdx];

    // Bind the global descriptor set to be updated.
    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            shader->pipeline.pipelineLayout, 0, 1,
                            &globalDescriptor, 0, 0);

    u32 range = sizeof(sceneUBO);
    u64 offset = 0;

    vulkanBufferLoadData(header, &shader->globalUniformBuffer, offset, range, 0,
                         &shader->globalUbo);

    VkDescriptorBufferInfo bufferInfo;
    bufferInfo.buffer = shader->globalUniformBuffer.buffer;
    bufferInfo.offset = offset;
    bufferInfo.range = range;

    // Update descriptor sets.
    VkWriteDescriptorSet newDescriptor = {
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    newDescriptor.dstSet = shader->globalDescriptorSets[header->imageIdx];
    newDescriptor.dstBinding = 0;
    newDescriptor.dstArrayElement = 0;
    newDescriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    newDescriptor.descriptorCount = 1;
    newDescriptor.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(header->device.logicalDevice, 1, &newDescriptor, 0,
                           0);
}

void vulkanUIShaderSetModel(vulkanHeader* header,
                            struct vulkanOverallShader* shader, mat4 model) {
    u32 imgIdx = header->imageIdx;
    vulkanCommandBuffer cb = header->graphicsCommandBuffers[imgIdx];
    vkCmdPushConstants(cb.handle, shader->pipeline.pipelineLayout,
                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(mat4), &model);
}

void vulkanUIShaderApplyMaterial(vulkanHeader* header,
                                 struct vulkanOverallShader* shader,
                                 material* material) {
    u32 imgIdx = header->imageIdx;
    vulkanCommandBuffer cb = header->graphicsCommandBuffers[imgIdx];

    vulkanOverallShaderState* objState =
        &shader->objectStates[material->shaderID];
    VkDescriptorSet objSet = objState->descriptorSets[imgIdx];

    VkWriteDescriptorSet
        descriptorWrites[VULKAN_OVERALL_SHADER_DESCRIPTOR_COUNT];
    fzeroMemory(descriptorWrites, sizeof(VkWriteDescriptorSet) *
                                      VULKAN_OVERALL_SHADER_DESCRIPTOR_COUNT);
    // The amount of descriptors updated
    u32 descriptorUpdatedCnt = 0;
    u32 descriptorIdx = 0;

    u32 range = sizeof(materialUBO);
    u64 offset = sizeof(materialUBO) * material->shaderID;
    materialUBO obo;
    // TEMP
    //  static f32 accum = 0.0f;
    //  accum += header->deltaTime;
    //  f32 s = (fsnSin(accum) + 1.0f) / 2.0f;
    obo.diffuse_color = material->diffuseColor;

    vulkanBufferLoadData(header, &shader->objectUniformBuffer, offset, range, 0,
                         &obo);

    // Only do this if the descriptor has not yet been updated.
    if (objState->descriptorStates[descriptorIdx].generations[imgIdx] ==
        INVALID_ID) {
        VkDescriptorBufferInfo buffer_info;
        buffer_info.buffer = shader->objectUniformBuffer.buffer;
        buffer_info.offset = offset;
        buffer_info.range = range;

        VkWriteDescriptorSet descriptor = {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        descriptor.dstSet = objSet;
        descriptor.dstBinding = descriptorIdx;
        descriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor.descriptorCount = 1;
        descriptor.pBufferInfo = &buffer_info;

        descriptorWrites[descriptorUpdatedCnt] = descriptor;
        descriptorUpdatedCnt++;

        // Update the frame generation. In this case it is only needed once
        // since this is a buffer.
        objState->descriptorStates[descriptorIdx].generations[imgIdx] = 1;
    }
    descriptorIdx++;

    // TODO: more samplers.
    const u32 samplerMaxCnt = 1;
    VkDescriptorImageInfo imgInfos[1];
    for (u32 samplerIdx = 0; samplerIdx < samplerMaxCnt; ++samplerIdx) {
        mapType type = shader->samplerTypes[samplerIdx];
        texture* t = 0;
        switch (type) {
            case TEXTURE_USE_MAP_DIFFUSE:
                t = material->diffuseMap.texture;
                break;
            default:
                FFATAL("Unable to bind sampler to unknown use.");
                return;
        }
        u32* descriptorGeneration =
            &objState->descriptorStates[descriptorIdx].generations[imgIdx];
        u32* descriptorID =
            &objState->descriptorStates[descriptorIdx].ids[imgIdx];

        if (t->generation == INVALID_ID) {
            t = textureSystemGetDefault();
            *descriptorGeneration = INVALID_ID;
        }

        // Check if the descriptor needs updating first.
        // If the descriptorID is not the same then the texture was swapped for
        // a different one. If the generations don't match then the
        // texture/material was updated
        if (t &&
            (*descriptorID != t->id || *descriptorGeneration != t->generation ||
             *descriptorGeneration == INVALID_ID)) {
            vulkanTextureData* internal_data = (vulkanTextureData*)t->data;

            // Assign view and sampler.
            imgInfos[samplerIdx].imageLayout =
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imgInfos[samplerIdx].imageView = internal_data->image.view;
            imgInfos[samplerIdx].sampler = internal_data->sampler;

            VkWriteDescriptorSet descriptor = {
                VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            descriptor.dstSet = objSet;
            descriptor.dstBinding = descriptorIdx;
            descriptor.descriptorType =
                VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptor.descriptorCount = 1;
            descriptor.pImageInfo = &imgInfos[samplerIdx];

            descriptorWrites[descriptorUpdatedCnt] = descriptor;
            descriptorUpdatedCnt++;

            // Sync frame generation/Ids if not using a default texture.
            if (t->generation != INVALID_ID) {
                *descriptorGeneration = t->generation;
                *descriptorID = t->id;
            }
            descriptorIdx++;
        }
    }

    if (descriptorUpdatedCnt > 0) {
        vkUpdateDescriptorSets(header->device.logicalDevice,
                               descriptorUpdatedCnt, descriptorWrites, 0, 0);
    }

    // Bind the descriptor set to be updated, or in case the shader changed.
    vkCmdBindDescriptorSets(cb.handle, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            shader->pipeline.pipelineLayout, 1, 1, &objSet, 0,
                            0);
}

b8 vulkanUIShaderResourceAcquire(vulkanHeader* header,
                                 struct vulkanOverallShader* shader,
                                 material* mat) {
    // TODO: free list
    mat->shaderID = shader->objectUniformBufferIdx;
    shader->objectUniformBufferIdx++;

    vulkanOverallShaderState* object_state =
        &shader->objectStates[mat->shaderID];
    for (u32 i = 0; i < VULKAN_OVERALL_SHADER_DESCRIPTOR_COUNT; ++i) {
        for (u32 j = 0; j < 3; ++j) {
            object_state->descriptorStates[i].generations[j] = INVALID_ID;
        }
    }

    // Allocate descriptor sets.
    VkDescriptorSetLayout layouts[3] = {shader->objectDescriptorSetLayout,
                                        shader->objectDescriptorSetLayout,
                                        shader->objectDescriptorSetLayout};

    VkDescriptorSetAllocateInfo alloc_info = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    alloc_info.descriptorPool = shader->objectDescriptorPool;
    alloc_info.descriptorSetCount = 3; // one per frame
    alloc_info.pSetLayouts = layouts;
    VkResult result =
        vkAllocateDescriptorSets(header->device.logicalDevice, &alloc_info,
                                 object_state->descriptorSets);
    if (result != VK_SUCCESS) {
        FERROR("Error allocating descriptor sets in shader!");
        return false;
    }

    return true;
}

void vulkanUIShaderResourceRelease(vulkanHeader* header,
                                   struct vulkanOverallShader* shader,
                                   material* mat) {
    vulkanOverallShaderState* object_state =
        &shader->objectStates[mat->shaderID];

    const u32 descriptor_set_count = 3;
    vkDeviceWaitIdle(header->device.logicalDevice);
    // Release object descriptor sets.
    VkResult result = vkFreeDescriptorSets(
        header->device.logicalDevice, shader->objectDescriptorPool,
        descriptor_set_count, object_state->descriptorSets);
    if (result != VK_SUCCESS) {
        FERROR("Error freeing object shader descriptor sets!");
    }

    for (u32 i = 0; i < VULKAN_OVERALL_SHADER_DESCRIPTOR_COUNT; ++i) {
        for (u32 j = 0; j < 3; ++j) {
            object_state->descriptorStates[i].generations[j] = INVALID_ID;
        }
    }

    // TODO: add the object_id to the free list
}
