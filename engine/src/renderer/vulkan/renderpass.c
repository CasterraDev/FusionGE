#include "renderpass.h"
#include "core/fmemory.h"

void vulkanRenderpassCreate(
    vulkanHeader* header, 
    vulkanRenderpass* outRenderpass,
    f32 x, f32 y, f32 w, f32 h,
    f32 r, f32 g, f32 b, f32 a,
    f32 depth,
    u32 stencil){
    
    outRenderpass->x = x;
    outRenderpass->y = y;
    outRenderpass->w = w;
    outRenderpass->h = h;
    
    outRenderpass->r = r;
    outRenderpass->g = g;
    outRenderpass->b = b;
    outRenderpass->a = a;

    outRenderpass->depth = depth;
    outRenderpass->stencil = stencil;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    // Attachments TODO: make this configurable.
    VkAttachmentDescription attachmentDescriptions[2];

    // Color attachment
    VkAttachmentDescription colorAttachment;
    colorAttachment.format = header->swapchain.imgFormat.format; // TODO: configurable
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;      // Do not expect any particular layout before render pass starts.
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  // Transitioned to after the render pass
    colorAttachment.flags = 0;

    attachmentDescriptions[0] = colorAttachment;

    VkAttachmentReference colorAttachmentRef;
    colorAttachmentRef.attachment = 0;  // Attachment description array index
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    // Depth attachment, if there is one
    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = header->device.depthFormat;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    attachmentDescriptions[1] = depthAttachment;

    // Depth attachment reference
    VkAttachmentReference depthAttachmentRef;
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // TODO: other attachment types (input, resolve, preserve)

    // Depth stencil data.
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    // Input from a shader
    subpass.inputAttachmentCount = 0;
    subpass.pInputAttachments = 0;

    // Attachments used for multisampling color attachments
    subpass.pResolveAttachments = 0;

    // Attachments not used in this subpass, but must be preserved for the next.
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments = 0;

    // Render pass dependencies. TODO: make this configurable.
    VkSubpassDependency dependency;
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependency.dependencyFlags = 0;

    // Render pass create.
    VkRenderPassCreateInfo renderPassCreateInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
    renderPassCreateInfo.attachmentCount = 2;
    renderPassCreateInfo.pAttachments = attachmentDescriptions;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &dependency;
    renderPassCreateInfo.pNext = 0;
    renderPassCreateInfo.flags = 0;

    VK_CHECK(vkCreateRenderPass(
        header->device.logicalDevice,
        &renderPassCreateInfo,
        header->allocator,
        &outRenderpass->handle));
}

void vulkanRenderpassDestroy(vulkanHeader* header, vulkanRenderpass* renderpass){
    if (renderpass && renderpass->handle){
        vkDestroyRenderPass(header->device.logicalDevice,renderpass->handle,header->allocator);
        renderpass->handle = 0;
    }
}

void vulkanRenderpassBegin(
    vulkanCommandBuffer* commandBuffer, 
    vulkanRenderpass* renderpass,
    VkFramebuffer frameBuffer){
    
    VkRenderPassBeginInfo beginInfo = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
    beginInfo.renderPass = renderpass->handle;
    beginInfo.framebuffer = frameBuffer;
    beginInfo.renderArea.offset.x = renderpass->x;
    beginInfo.renderArea.offset.y = renderpass->y;
    beginInfo.renderArea.extent.width = renderpass->w;
    beginInfo.renderArea.extent.height = renderpass->h;

    VkClearValue clearVals[2];
    fzeroMemory(clearVals, sizeof(VkClearValue) * 2);
    clearVals[0].color.float32[0] = renderpass->r;
    clearVals[0].color.float32[1] = renderpass->g;
    clearVals[0].color.float32[2] = renderpass->b;
    clearVals[0].color.float32[3] = renderpass->a;
    clearVals[1].depthStencil.depth = renderpass->depth;
    clearVals[1].depthStencil.stencil = renderpass->stencil;

    beginInfo.clearValueCount = 2;
    beginInfo.pClearValues = clearVals;

    vkCmdBeginRenderPass(commandBuffer->handle, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
    commandBuffer->state = COMMAND_BUFFER_STATE_IN_RENDER_PASS;
}

void vulkanRenderpassEnd(vulkanCommandBuffer* commandBuffer, vulkanRenderpass* renderpass){
    vkCmdEndRenderPass(commandBuffer->handle);
    commandBuffer->state = COMMAND_BUFFER_STATE_RECORDING;
}
