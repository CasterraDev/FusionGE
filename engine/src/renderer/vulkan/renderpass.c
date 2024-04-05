#include "renderpass.h"
#include "core/fmemory.h"

void vulkanRenderpassCreate(
    vulkanHeader* header, 
    vulkanRenderpass* outRenderpass,
    vector4 renderArea,
    vector4 clearColor,
    f32 depth,
    u32 stencil,
    u8 clearFlags,
    b8 hadPrev,
    b8 hasNext){
    outRenderpass->clearFlags = clearFlags;
    outRenderpass->renderArea = renderArea;
    outRenderpass->clearColor = clearColor;
    outRenderpass->hadPrev = hadPrev;
    outRenderpass->hasNext = hasNext;

    outRenderpass->depth = depth;
    outRenderpass->stencil = stencil;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

    // Attachments TODO: make this configurable.
    //TODO: Make attachments attach conditionally
    u32 attachmentDescriptionCnt = 0;
    VkAttachmentDescription attachmentDescriptions[2];

    // Color attachment
    b8 shouldClearColor = (outRenderpass->clearFlags & RENDERPASS_CLEAR_COLOR_BUFFER_FLAG) != 0;
    VkAttachmentDescription colorAttachment;
    colorAttachment.format = header->swapchain.imgFormat.format; // TODO: configurable
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = shouldClearColor ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = hadPrev ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_UNDEFINED;      // Do not expect any particular layout before render pass starts.
    colorAttachment.finalLayout = hasNext ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;  // Transitioned to after the render pass
    colorAttachment.flags = 0;

    attachmentDescriptions[attachmentDescriptionCnt] = colorAttachment;
    attachmentDescriptionCnt++;

    VkAttachmentReference colorAttachmentRef;
    colorAttachmentRef.attachment = 0;  // Attachment description array index
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    // Depth attachment, if there is one
    b8 shouldClearDepth = (outRenderpass->clearFlags & RENDERPASS_CLEAR_DEPTH_BUFFER_FLAG) != 0;
    if (shouldClearDepth){
        VkAttachmentDescription depthAttachment = {};
        depthAttachment.format = header->device.depthFormat;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = shouldClearDepth ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        
        attachmentDescriptions[attachmentDescriptionCnt] = depthAttachment;
        attachmentDescriptionCnt++;

        // Depth attachment reference
        VkAttachmentReference depthAttachmentRef;
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


        // Depth stencil data.
        subpass.pDepthStencilAttachment = &depthAttachmentRef;
    }else{
        fzeroMemory(&attachmentDescriptions[attachmentDescriptionCnt], sizeof(VkAttachmentDescription));
        subpass.pDepthStencilAttachment = 0;
    }
    // TODO: other attachment types (input, resolve, preserve)

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
    renderPassCreateInfo.attachmentCount = attachmentDescriptionCnt;
    renderPassCreateInfo.pAttachments = attachmentDescriptions;
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;
    renderPassCreateInfo.dependencyCount = 1;
    renderPassCreateInfo.pDependencies = &dependency;
    renderPassCreateInfo.pNext = 0;
    renderPassCreateInfo.flags = 0;

    VULKANSUCCESS(vkCreateRenderPass(
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
    beginInfo.renderArea.offset.x = renderpass->renderArea.x;
    beginInfo.renderArea.offset.y = renderpass->renderArea.y;
    beginInfo.renderArea.extent.width = renderpass->renderArea.z;
    beginInfo.renderArea.extent.height = renderpass->renderArea.w;

    beginInfo.clearValueCount = 0;
    beginInfo.pClearValues = 0;

    VkClearValue clearVals[2];
    fzeroMemory(clearVals, sizeof(VkClearValue) * 2);
    b8 shouldClearColor = (renderpass->clearFlags & RENDERPASS_CLEAR_COLOR_BUFFER_FLAG) != 0;
    if (shouldClearColor){
        fcopyMemory(clearVals[beginInfo.clearValueCount].color.float32, renderpass->clearColor.elements, sizeof(f32) * 4);
        beginInfo.clearValueCount++;
    }

    b8 shouldClearDepth = (renderpass->clearFlags & RENDERPASS_CLEAR_DEPTH_BUFFER_FLAG) != 0;
    if (shouldClearDepth){
        fcopyMemory(clearVals[beginInfo.clearValueCount].color.float32, renderpass->clearColor.elements, sizeof(f32) * 4);
        clearVals[beginInfo.clearValueCount].depthStencil.depth = renderpass->depth;

        b8 shouldClearStencil = (renderpass->clearFlags & RENDERPASS_CLEAR_STENCIL_BUFFER_FLAG) != 0;
        clearVals[beginInfo.clearValueCount].depthStencil.stencil = shouldClearStencil ? renderpass->stencil : 0;

        beginInfo.clearValueCount++;
    }

    beginInfo.pClearValues = beginInfo.clearValueCount > 0 ? clearVals : 0;

    vkCmdBeginRenderPass(commandBuffer->handle, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);
    commandBuffer->state = COMMAND_BUFFER_STATE_IN_RENDER_PASS;
}

void vulkanRenderpassEnd(vulkanCommandBuffer* commandBuffer, vulkanRenderpass* renderpass){
    vkCmdEndRenderPass(commandBuffer->handle);
    commandBuffer->state = COMMAND_BUFFER_STATE_RECORDING;
}
