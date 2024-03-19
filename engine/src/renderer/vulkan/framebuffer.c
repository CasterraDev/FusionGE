#include "framebuffer.h"
#include "core/fmemory.h"

void vulkanFramebufferCreate(vulkanHeader* header, vulkanRenderpass* renderpass, u32 width, u32 height, u32 attachmentCnt, VkImageView* attachments, vulkanFramebuffer* outFramebuffer){
    outFramebuffer->attachments = fallocate(sizeof(VkImageView) * attachmentCnt, MEMORY_TAG_RENDERER);
    for (u32 i = 0; i < attachmentCnt; ++i){
        outFramebuffer->attachments[i] = attachments[i];
    }
    outFramebuffer->renderpass = renderpass;
    outFramebuffer->attachmentCnt = attachmentCnt;

    VkFramebufferCreateInfo framebufferCreateInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    framebufferCreateInfo.renderPass = renderpass->handle;
    framebufferCreateInfo.attachmentCount = attachmentCnt;
    framebufferCreateInfo.pAttachments = outFramebuffer->attachments;
    framebufferCreateInfo.width = width;
    framebufferCreateInfo.height = height;
    framebufferCreateInfo.layers = 1;

    VK_CHECK(vkCreateFramebuffer(header->device.logicalDevice, &framebufferCreateInfo, header->allocator, &outFramebuffer->handle));
}

void vulkanFramebufferDestroy(vulkanHeader* header, vulkanFramebuffer* framebuffer){
    vkDestroyFramebuffer(header->device.logicalDevice, framebuffer->handle, header->allocator);
    if (framebuffer->attachments){
        ffree(framebuffer->attachments, sizeof(VkImageView) * framebuffer->attachmentCnt, MEMORY_TAG_RENDERER);
        framebuffer->attachments = 0;
    }
    framebuffer->handle = 0;
    framebuffer->attachmentCnt = 0;
    framebuffer->renderpass = 0;
}