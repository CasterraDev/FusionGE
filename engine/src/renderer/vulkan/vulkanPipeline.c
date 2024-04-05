#include "vulkanPipeline.h"
#include "math/matrixMath.h"
#include "core/fmemory.h"

b8 vulkanPipelineCreate(vulkanHeader* header, vulkanPipelineConfig* config, vulkanPipeline* outPipeline){
    // Viewport state
    VkPipelineViewportStateCreateInfo viewportState = {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewportState.viewportCount = 1;
    viewportState.pViewports = &config->viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &config->scissor;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    rasterizerCreateInfo.depthClampEnable = VK_FALSE;
    rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterizerCreateInfo.polygonMode = config->isWireframe ? VK_POLYGON_MODE_LINE : VK_POLYGON_MODE_FILL;
    rasterizerCreateInfo.lineWidth = 1.0f;
    rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizerCreateInfo.depthBiasEnable = VK_FALSE;
    rasterizerCreateInfo.depthBiasConstantFactor = 0.0f;
    rasterizerCreateInfo.depthBiasClamp = 0.0f;
    rasterizerCreateInfo.depthBiasSlopeFactor = 0.0f;

    // Multisampling.
    VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisamplingCreateInfo.sampleShadingEnable = VK_FALSE;
    multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisamplingCreateInfo.minSampleShading = 1.0f;
    multisamplingCreateInfo.pSampleMask = 0;
    multisamplingCreateInfo.alphaToCoverageEnable = VK_FALSE;
    multisamplingCreateInfo.alphaToOneEnable = VK_FALSE;

    // Depth and stencil testing.
    VkPipelineDepthStencilStateCreateInfo depthStencil = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    if (config->shouldDepthTest){
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;
    }

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState;
    fzeroMemory(&colorBlendAttachmentState, sizeof(VkPipelineColorBlendAttachmentState));
    colorBlendAttachmentState.blendEnable = VK_TRUE;
    colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

    colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                                  VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendStateCreateInfo.attachmentCount = 1;
    colorBlendStateCreateInfo.pAttachments = &colorBlendAttachmentState;

    // Dynamic state
    VkDynamicState dynamicStates[3] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_LINE_WIDTH};

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamicStateCreateInfo.dynamicStateCount = 3;
    dynamicStateCreateInfo.pDynamicStates = dynamicStates;

    // Vertex input
    VkVertexInputBindingDescription bindingDescription;
    bindingDescription.binding = 0;  // Binding index
    bindingDescription.stride = config->stride;
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;  // Move to next data entry for each vertex.

    // Attributes
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = config->attrCnt;
    vertexInputInfo.pVertexAttributeDescriptions = config->attributes;

    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};

    //Push Consts
    VkPushConstantRange pushConst;
    pushConst.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConst.offset = 0;
    pushConst.size = sizeof(mat4) * 2;

    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &pushConst;

    // Descriptor set layouts
    pipelineLayoutCreateInfo.setLayoutCount = config->descriptorSetLayoutCnt;
    pipelineLayoutCreateInfo.pSetLayouts = config->descriptorSetLayouts;

    VULKANSUCCESS(vkCreatePipelineLayout(header->device.logicalDevice, &pipelineLayoutCreateInfo, header->allocator, &outPipeline->pipelineLayout));

    //TODO: look more into VkPipelineCache and maybe add one

    VkGraphicsPipelineCreateInfo gPipelineCreateInfo = {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    gPipelineCreateInfo.stageCount = config->stageCnt;
    gPipelineCreateInfo.pStages = config->stages;
    gPipelineCreateInfo.renderPass = config->renderpass->handle;

    gPipelineCreateInfo.pInputAssemblyState = &inputAssembly;
    gPipelineCreateInfo.pVertexInputState = &vertexInputInfo;
    gPipelineCreateInfo.pViewportState = &viewportState;
    gPipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
    gPipelineCreateInfo.pColorBlendState = &colorBlendStateCreateInfo;
    gPipelineCreateInfo.pDepthStencilState = config->shouldDepthTest ? &depthStencil : 0;
    gPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
    gPipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
    gPipelineCreateInfo.pTessellationState = 0;

    gPipelineCreateInfo.layout = outPipeline->pipelineLayout;

    VULKANSUCCESS(vkCreateGraphicsPipelines(header->device.logicalDevice, NULL, 1, &gPipelineCreateInfo, header->allocator, &outPipeline->handle));

    return true;
}

void vulkanPipelineDestroy(vulkanHeader* header, vulkanPipeline* pipeline){
    if (pipeline){
        if (pipeline->handle){
            vkDestroyPipeline(header->device.logicalDevice, pipeline->handle, header->allocator);
            pipeline->handle = 0;
        }
        if (pipeline->pipelineLayout){
            vkDestroyPipelineLayout(header->device.logicalDevice, pipeline->pipelineLayout, header->allocator);
            pipeline->pipelineLayout = 0;
        }
    }
}

void vulkanPipelineBind(vulkanCommandBuffer* cb, VkPipelineBindPoint bindPoint, vulkanPipeline* pipeline) {
    vkCmdBindPipeline(cb->handle, bindPoint, pipeline->handle);
}
