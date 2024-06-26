#include "backend.h"
#include "commandBuffer.h"
#include "defines.h"
#include "renderer/vulkan/shaders/vulkanShadersUtil.h"
#include "renderpass.h"
#include "resources/resourceManager.h"
#include "systems/shaderSystem.h"
#include "utils.h"
#include "vulkanBuffers.h"
#include "vulkanDevice.h"
#include "vulkanHeader.h"
#include "vulkanImage.h"
#include "vulkanPlatform.h"
#include "vulkanSwapchain.h"

#include "core/application.h"
#include "core/fmemory.h"
#include "core/fstring.h"
#include "core/logger.h"
#include "vulkanPipeline.h"

#include "helpers/dinoArray.h"

#include "math/matrixMath.h"

#include "platform/platform.h"
#include "resources/resourcesTypes.h"
#include "systems/geometrySystem.h"
#include "systems/materialSystem.h"
#include "systems/textureSystem.h"
#include <vulkan/vulkan_core.h>

static vulkanHeader header;
static u32 cachedFramebufferWidth = 0;
static u32 cachedFramebufferHeight = 0;

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData);

i32 findMemoryIdx(u32 typeFilter, u32 propertyFlags);

void createCommandBuffers(rendererBackend* backend);
void regenerateFramebuffers();

b8 recreateSwapchain(rendererBackend* backend);

b8 uploadDataViaStagingBuffer(vulkanHeader* header, VkCommandPool pool,
                              VkFence fence, VkQueue queue,
                              vulkanBuffer* buffer, u64* outOffset, u64 size,
                              const void* data) {
    // Allocate space in the buffer
    if (!vulkanBufferAllocate(buffer, size, outOffset)) {
        FERROR("uploadDataViaStagingBuffer failed.\n");
        return false;
    }
    vulkanBuffer vb;
    vulkanBufferCreate(header, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                       true, false, &vb);

    vulkanBufferLoadData(header, &vb, 0, size, 0, data);

    // Vulkan copy buffers
    vulkanBufferCopyTo(header, pool, fence, queue, vb.buffer, 0, buffer->buffer,
                       *outOffset, size);

    vulkanBufferDestroy(header, &vb);
    return true;
}

b8 createBuffers(vulkanHeader* header) {
    VkMemoryPropertyFlagBits memoryPropertyFlags =
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    const u64 vertexBufferSize = sizeof(vertex3D) * 1024 * 1024;
    if (!vulkanBufferCreate(header, vertexBufferSize,
                            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                            memoryPropertyFlags, true, true,
                            &header->objectVertexBuffer)) {
        FERROR("Error creating vertex buffer.");
        return false;
    }

    const u64 indexBufferSize = sizeof(u32) * 1024 * 1024;
    if (!vulkanBufferCreate(header, indexBufferSize,
                            VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                            memoryPropertyFlags, true, true,
                            &header->objectIndexBuffer)) {
        FERROR("Error creating vertex buffer.");
        return false;
    }

    return true;
}

b8 vulkanInit(struct rendererBackend* backend, const char* appName) {

    header.findMemoryIdx = findMemoryIdx;
    // TODO: Custom allocator
    header.allocator = 0;

    appGetFramebufferSize(&cachedFramebufferWidth, &cachedFramebufferHeight);
    header.framebufferWidth =
        (cachedFramebufferWidth != 0) ? cachedFramebufferWidth : 800;
    header.framebufferHeight =
        (cachedFramebufferHeight != 0) ? cachedFramebufferHeight : 600;
    cachedFramebufferWidth = 0;
    cachedFramebufferHeight = 0;

    VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.pApplicationName = appName;
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "Fusion Game Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instanceInfo = {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instanceInfo.pApplicationInfo = &appInfo;

    //--------Vulkan Extensions--------
    const char** requiredExts = dinoCreate(const char**);
    dinoPush(requiredExts, &VK_KHR_SURFACE_EXTENSION_NAME);
    platformGetRequiredExts(&requiredExts);

// Debug vulkan extensions
#if defined(_DEBUG)
    dinoPush(requiredExts, &VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    FDEBUG("Required extensions:");
    u32 length = dinoLength(requiredExts);
    for (u32 i = 0; i < length; ++i) {
        FDEBUG(requiredExts[i]);
    }
#endif

    // Validation Layers
    const char** requiredValLayers = 0;
    u32 requiredValLayersCnt = 0;

#if defined(_DEBUG)
    requiredValLayers = dinoCreate(const char**);
    dinoPush(requiredValLayers, &"VK_LAYER_KHRONOS_validation");
    requiredValLayersCnt = dinoLength(requiredValLayers);

    // Obtain a list of available validation layers
    u32 availLayerCnt = 0;
    VULKANSUCCESS(vkEnumerateInstanceLayerProperties(&availLayerCnt, 0));
    VkLayerProperties* availLayers =
        dinoCreateReserve(availLayerCnt, VkLayerProperties);
    VULKANSUCCESS(
        vkEnumerateInstanceLayerProperties(&availLayerCnt, availLayers));

    // Verify all required layers are available.
    for (u32 i = 0; i < requiredValLayersCnt; ++i) {
        FINFO("Searching for layer: %s", requiredValLayers[i]);
        b8 found = false;
        for (u32 j = 0; j < availLayerCnt; ++j) {
            if (strEqual(requiredValLayers[i], availLayers[j].layerName)) {
                found = true;
                FINFO("Found.");
                break;
            }
        }

        if (!found) {
            FFATAL("Required validation layer is missing: %s",
                   requiredValLayers[i]);
            return false;
        }
    }
    dinoDestroy(availLayers);
    FINFO("All required validation layers are present.");
#endif //_Debug

    instanceInfo.enabledLayerCount = requiredValLayersCnt;
    instanceInfo.ppEnabledLayerNames = requiredValLayers;
    instanceInfo.enabledExtensionCount = dinoLength(requiredExts);
    instanceInfo.ppEnabledExtensionNames = requiredExts;

    // TODO: Create custom allocater
    VULKANSUCCESS(
        vkCreateInstance(&instanceInfo, header.allocator, &header.instance));
    FINFO("Vulkan Instance Created")

    // Clear up the dinos
    dinoDestroy(requiredExts);
#if defined(_DEBUG)
    dinoDestroy(requiredValLayers);
#endif

    // Debugger
#if defined(_DEBUG)
    FDEBUG("Creating Vulkan debugger...");
    u32 log_severity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT; //|
                                                      //    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    debugCreateInfo.messageSeverity = log_severity;
    debugCreateInfo.messageType =
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debugCreateInfo.pfnUserCallback = vk_debugCallback;

    PFN_vkCreateDebugUtilsMessengerEXT func =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
            header.instance, "vkCreateDebugUtilsMessengerEXT");
    FASSERT_MSG(func, "Failed to create debug messenger!");
    VULKANSUCCESS(func(header.instance, &debugCreateInfo, header.allocator,
                       &header.debugMessenger));
    FDEBUG("Vulkan debugger created.");
#endif

    // Create surface
    if (!platformCreateVulkanSurface(&header)) {
        return false;
    }
    FDEBUG("Vulkan surface created");

    // Create Device
    if (!getVulkanDevice(&header)) {
        FFATAL("Couldn't get physical device");
        return false;
    }

    // Create Swapchain
    vulkanSwapchainCreate(&header, header.framebufferWidth,
                          header.framebufferHeight, &header.swapchain);

    //------------------Create Renderpasses
    // World Renderpass
    vulkanRenderpassCreate(
        &header, &header.mainRenderpass,
        (vector4){0, 0, header.framebufferWidth, header.framebufferHeight},
        (vector4){0.0f, 0.0f, 0.2f, 1.0f}, 1.0f, 0,
        RENDERPASS_CLEAR_COLOR_BUFFER_FLAG |
            RENDERPASS_CLEAR_DEPTH_BUFFER_FLAG |
            RENDERPASS_CLEAR_STENCIL_BUFFER_FLAG,
        false, true);
    // UI Renderpass
    vulkanRenderpassCreate(
        &header, &header.uiRenderpass,
        (vector4){0, 0, header.framebufferWidth, header.framebufferHeight},
        (vector4){0.0f, 0.0f, 0.2f, 1.0f}, 1.0f, 0, RENDERPASS_CLEAR_NONE_FLAG,
        true, false);

    // Swapchain framebuffers
    regenerateFramebuffers();

    // Create Command Buffers
    createCommandBuffers(backend);

    // Create Sync Objects
    header.imageAvailSemaphores =
        dinoCreateReserve(header.swapchain.maxNumOfFramesInFlight, VkSemaphore);
    header.queueCompleteSemaphores =
        dinoCreateReserve(header.swapchain.maxNumOfFramesInFlight, VkSemaphore);

    for (u8 i = 0; i < header.swapchain.maxNumOfFramesInFlight; ++i) {
        VkSemaphoreCreateInfo semaphoreCreateInfo = {
            VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        vkCreateSemaphore(header.device.logicalDevice, &semaphoreCreateInfo,
                          header.allocator, &header.imageAvailSemaphores[i]);
        vkCreateSemaphore(header.device.logicalDevice, &semaphoreCreateInfo,
                          header.allocator, &header.queueCompleteSemaphores[i]);

        // Create the fence in a signaled state, indicating that the first frame
        // has already been "rendered". This will prevent the app from waiting
        // indefinitely for the first frame to render since it Cannot be
        // rendered until a frame is "rendered" before it
        VkFenceCreateInfo fenceCI = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VULKANSUCCESS(vkCreateFence(header.device.logicalDevice, &fenceCI,
                                    header.allocator,
                                    &header.inFlightFences[i]));
    }

    // In flight fences should not yet exist at this point, so clear the list.
    // These are stored in pointers because the initial state should be 0, and
    // will be 0 when not in use. Acutal fences are not owned by this list.
    for (u32 i = 0; i < header.swapchain.imageCnt; ++i) {
        header.imagesInFlight[i] = 0;
    }

    // Create buffers
    createBuffers(&header);

    // Mark all geometries as invalid
    for (u32 i = 0; i < VULKAN_MAX_GEOMETRY_COUNT; ++i) {
        header.geometries[i].id = INVALID_ID;
    }

    FINFO("Vulkan Rendering subsystem inited");
    return true;
}

void vulkanShutdown(struct rendererBackend* backend) {
    vkDeviceWaitIdle(header.device.logicalDevice);

    // Destroy in the opposite order of creation.

    vulkanBufferDestroy(&header, &header.objectVertexBuffer);
    vulkanBufferDestroy(&header, &header.objectIndexBuffer);

    // Sync Objects
    for (u8 i = 0; i < header.swapchain.maxNumOfFramesInFlight; ++i) {
        if (header.imageAvailSemaphores[i]) {
            vkDestroySemaphore(header.device.logicalDevice,
                               header.imageAvailSemaphores[i],
                               header.allocator);
            header.imageAvailSemaphores[i] = 0;
        }
        if (header.queueCompleteSemaphores[i]) {
            vkDestroySemaphore(header.device.logicalDevice,
                               header.queueCompleteSemaphores[i],
                               header.allocator);
            header.queueCompleteSemaphores[i] = 0;
        }
        vkDestroyFence(header.device.logicalDevice, header.inFlightFences[i],
                       header.allocator);
    }
    dinoDestroy(header.imageAvailSemaphores);
    header.imageAvailSemaphores = 0;

    dinoDestroy(header.queueCompleteSemaphores);
    header.queueCompleteSemaphores = 0;

    // Command Buffers
    for (u32 i = 0; i < header.swapchain.imageCnt; ++i) {
        if (header.graphicsCommandBuffers[i].handle) {
            vulkanCommandBufferFree(&header, header.device.graphicsCommandPool,
                                    &header.graphicsCommandBuffers[i]);
            header.graphicsCommandBuffers[i].handle = 0;
        }
    }
    dinoDestroy(header.graphicsCommandBuffers);
    header.graphicsCommandBuffers = 0;

    // Destroy Framebuffers
    for (u32 i = 0; i < header.swapchain.imageCnt; ++i) {
        vkDestroyFramebuffer(header.device.logicalDevice,
                             header.worldFrameBuffers[i], header.allocator);
        vkDestroyFramebuffer(header.device.logicalDevice,
                             header.swapchain.framebuffers[i],
                             header.allocator);
    }

    // Renderpass
    vulkanRenderpassDestroy(&header, &header.uiRenderpass);
    vulkanRenderpassDestroy(&header, &header.mainRenderpass);

    // Swapchain
    vulkanSwapchainDestroy(&header, &header.swapchain);

    // Device
    vulkanDeviceDestroy(&header);

    if (header.surface) {
        vkDestroySurfaceKHR(header.instance, header.surface, header.allocator);
        header.surface = 0;
    }

#if defined(_DEBUG)
    FDEBUG("Destroying Vulkan debugger...");
    if (header.debugMessenger) {
        PFN_vkDestroyDebugUtilsMessengerEXT func =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
                header.instance, "vkDestroyDebugUtilsMessengerEXT");
        func(header.instance, header.debugMessenger, header.allocator);
    }
#endif
    FDEBUG("Destroying Vulkan instance...");
    vkDestroyInstance(header.instance, header.allocator);
}

void vulkanResized(struct rendererBackend* backend, u16 width, u16 height) {
    // Update the "framebuffer size generation", a counter which indicates when
    // the framebuffer size has been updated
    cachedFramebufferWidth = width;
    cachedFramebufferHeight = height;
    header.framebufferSizeGeneration++;

    FINFO("Vulkan renderer backend->resized: w/h/gen: %i/%i/%llu", width,
          height, header.framebufferSizeGeneration);
}

void vulkanDrawGeometry(geometryRenderData data) {
    vulkanGeometryData* bd = &header.geometries[data.geometry->internalID];
    vulkanCommandBuffer* cb = &header.graphicsCommandBuffers[header.imageIdx];
    // vulkanMaterialShaderUse(&header, &header.materialShader);

    // Bind vertex buffer at offset.
    VkDeviceSize offsets[1] = {bd->vertexBufferInfo.bufferOffset};
    vkCmdBindVertexBuffers(cb->handle, 0, 1, &header.objectVertexBuffer.buffer,
                           (VkDeviceSize*)offsets);

    if (bd->indexBufferInfo.count > 0) {
        // Bind index buffer at offset.
        vkCmdBindIndexBuffer(cb->handle, header.objectIndexBuffer.buffer,
                             bd->indexBufferInfo.bufferOffset,
                             VK_INDEX_TYPE_UINT32);
        // Issue the draw.
        vkCmdDrawIndexed(cb->handle, bd->indexBufferInfo.count, 1, 0, 0, 0);
    } else {
        vkCmdDraw(cb->handle, bd->vertexBufferInfo.count, 1, 0, 0);
    }
}

b8 vulkanBeginFrame(struct rendererBackend* backend, f32 deltaTime) {
    header.deltaTime = deltaTime;
    vulkanDevice* dev = &header.device;
    if (header.recreatingSwapchain) {
        VkResult res = vkDeviceWaitIdle(dev->logicalDevice);
        if (!successfullVulkanResult(res)) {
            FERROR("Vulkan renderer begin frame vkDeviceWaitIdle failed: %s",
                   vulkanResultStr(res, true));
            return false;
        }
        FINFO("Recreating swapchain...");
        return false;
    }

    // Check if the window has been resized
    if (header.framebufferSizeGeneration !=
        header.framebufferSizeLastGeneration) {
        VkResult res = vkDeviceWaitIdle(dev->logicalDevice);
        if (!successfullVulkanResult(res)) {
            FERROR("Vulkan renderer begin frame vkDeviceWaitIdle(2) failed: %s",
                   vulkanResultStr(res, true));
            return false;
        }

        // If the swapchain recreation failed (cuz the window was minimized, or
        // paused in some way) boot out before unsetting the flag
        if (!recreateSwapchain(backend)) {
            return false;
        }

        FINFO("Resized, booting");
        return false;
    }

    VkResult result = vkWaitForFences(
        header.device.logicalDevice, 1,
        &header.inFlightFences[header.currentFrame], true, UINT64_MAX);
    if (!successfullVulkanResult(result)) {
        FERROR("In flight fence wait failed. %s",
               vulkanResultStr(result, true));
        return false;
    }

    if (!vulkanSwapchainGetNextImgIdx(
            &header, &header.swapchain, UINT64_MAX,
            header.imageAvailSemaphores[header.currentFrame], 0,
            &header.imageIdx)) {
        return false;
    }

    // Begin recording commands
    vulkanCommandBuffer* cb = &header.graphicsCommandBuffers[header.imageIdx];
    vulkanCommandBufferReset(cb);
    vulkanCommandBufferBegin(cb, false, false, false);

    // Dynamic state
    // Start at the bottom left corner
    VkViewport vp;
    vp.x = 0.0f;
    vp.y = (f32)header.framebufferHeight;
    vp.width = (f32)header.framebufferWidth;
    vp.height = -(f32)header.framebufferHeight;
    vp.minDepth = 0.0f;
    vp.maxDepth = 1.0f;

    VkRect2D scissor;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    scissor.extent.width = header.framebufferWidth;
    scissor.extent.height = header.framebufferHeight;

    vkCmdSetViewport(cb->handle, 0, 1, &vp);
    vkCmdSetScissor(cb->handle, 0, 1, &scissor);

    header.mainRenderpass.renderArea.z = header.framebufferWidth;
    header.mainRenderpass.renderArea.w = header.framebufferHeight;
    header.uiRenderpass.renderArea.z = header.framebufferWidth;
    header.uiRenderpass.renderArea.w = header.framebufferHeight;

    return true;
}

b8 vulkanEndFrame(struct rendererBackend* backend, f32 deltaTime) {
    vulkanCommandBuffer* cb = &header.graphicsCommandBuffers[header.imageIdx];

    vulkanCommandBufferEnd(cb);

    if (header.imagesInFlight[header.imageIdx] != VK_NULL_HANDLE) {
        VkResult result = vkWaitForFences(
            header.device.logicalDevice, 1,
            &header.inFlightFences[header.currentFrame], true, UINT64_MAX);
        if (!successfullVulkanResult(result)) {
            FFATAL("In flight fence wait failed. %s",
                   vulkanResultStr(result, true));
            return false;
        }
    }

    // Mark the image fence as in-use by this frame
    header.imagesInFlight[header.imageIdx] =
        &header.inFlightFences[header.currentFrame];

    // Reset the fence for use on the next frame
    VULKANSUCCESS(vkResetFences(header.device.logicalDevice, 1,
                                &header.inFlightFences[header.currentFrame]));

    VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    // Command buffer(s) to be executed.
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cb->handle;
    // The semaphore(s) to be signaled when the queue is complete
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores =
        &header.queueCompleteSemaphores[header.currentFrame];
    // Wait semaphore ensures that the operation cannot begin until the image is
    // available.
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores =
        &header.imageAvailSemaphores[header.currentFrame];

    // Each semaphore waits to ensure only one is shown (presented) at a time
    VkPipelineStageFlags flags[1] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.pWaitDstStageMask = flags;

    VkResult res = vkQueueSubmit(header.device.graphicsQueue, 1, &submitInfo,
                                 header.inFlightFences[header.currentFrame]);
    if (!successfullVulkanResult(res)) {
        FERROR("vkQueueSubmit failed with a result of: %s",
               vulkanResultStr(res, true));
        return false;
    }

    vulkanCommandBufferUpdateSubmitted(cb);
    //-----------------End queue submission----------------

    // Give the image back to the swapchain
    vulkanSwapchainPresent(
        &header, &header.swapchain, header.device.graphicsQueue,
        header.device.presentQueue,
        header.queueCompleteSemaphores[header.currentFrame], header.imageIdx);

    return true;
}

b8 vulkanBeginRenderpass(struct rendererBackend* backend, u8 renderpassID) {
    VkFramebuffer fb = 0;
    vulkanRenderpass* rp = 0;
    vulkanCommandBuffer* cb = &header.graphicsCommandBuffers[header.imageIdx];

    switch (renderpassID) {
        case BUILTIN_RENDERPASS_WORLD: {
            fb = header.worldFrameBuffers[header.imageIdx];
            rp = &header.mainRenderpass;
            break;
        }
        case BUILTIN_RENDERPASS_UI: {
            fb = header.swapchain.framebuffers[header.imageIdx];
            rp = &header.uiRenderpass;
            break;
        }
        default: {
            FWARN("Begining renderpass of unknown id failed. %#02x",
                  renderpassID);
            return false;
        }
    }

    vulkanRenderpassBegin(cb, rp, fb);

    return true;
}

b8 vulkanEndRenderpass(struct rendererBackend* backend, u8 renderpassID) {
    vulkanRenderpass* rp = 0;
    vulkanCommandBuffer* cb = &header.graphicsCommandBuffers[header.imageIdx];

    switch (renderpassID) {
        case BUILTIN_RENDERPASS_WORLD: {
            rp = &header.mainRenderpass;
            break;
        }
        case BUILTIN_RENDERPASS_UI: {
            rp = &header.uiRenderpass;
            break;
        }
        default: {
            FWARN("Ending renderpass of unknown id failed. %#02x",
                  renderpassID);
            return false;
        }
    }

    vulkanRenderpassEnd(cb, rp);
    return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData, void* userData) {
    switch (messageSeverity) {
        default:
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
            FERROR(callbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
            FWARN(callbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
            FINFO(callbackData->pMessage);
            break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            FTRACE(callbackData->pMessage);
            break;
    }
    return VK_FALSE;
}

i32 findMemoryIdx(u32 typeFilter, u32 propertyFlags) {
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(header.device.physicalDevice,
                                        &memoryProperties);

    for (u32 i = 0; i < memoryProperties.memoryTypeCount; ++i) {
        // Check each memory type to see if its bit is set to 1.
        if (typeFilter & (1 << i) &&
            (memoryProperties.memoryTypes[i].propertyFlags & propertyFlags) ==
                propertyFlags) {
            return i;
        }
    }

    FWARN("Unable to find suitable memory type!");
    return -1;
}

void createCommandBuffers(rendererBackend* backend) {
    if (!header.graphicsCommandBuffers) {
        header.graphicsCommandBuffers =
            dinoCreateReserve(header.swapchain.imageCnt, vulkanCommandBuffer);
        for (u32 i = 0; i < header.swapchain.imageCnt; ++i) {
            fzeroMemory(&header.graphicsCommandBuffers[i],
                        sizeof(vulkanCommandBuffer));
        }
    }

    for (u32 i = 0; i < header.swapchain.imageCnt; ++i) {
        if (header.graphicsCommandBuffers[i].handle) {
            vulkanCommandBufferFree(&header, header.device.graphicsCommandPool,
                                    &header.graphicsCommandBuffers[i]);
        }
        fzeroMemory(&header.graphicsCommandBuffers[i],
                    sizeof(vulkanCommandBuffer));
        vulkanCommandBufferAllocate(&header, header.device.graphicsCommandPool,
                                    true, &header.graphicsCommandBuffers[i]);
    }

    FINFO("Vulkan command buffers created.");
}

void regenerateFramebuffers() {
    u32 cnt = header.swapchain.imageCnt;
    for (u32 i = 0; i < cnt; ++i) {
        VkImageView worldAttachments[2] = {
            header.swapchain.views[i], header.swapchain.depthAttachment.view};

        VkFramebufferCreateInfo worldFBCI = {
            VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        worldFBCI.renderPass = header.mainRenderpass.handle;
        worldFBCI.attachmentCount = 2;
        worldFBCI.pAttachments = worldAttachments;
        worldFBCI.width = header.framebufferWidth;
        worldFBCI.height = header.framebufferHeight;
        worldFBCI.layers = 1;

        VULKANSUCCESS(vkCreateFramebuffer(header.device.logicalDevice,
                                          &worldFBCI, header.allocator,
                                          &header.worldFrameBuffers[i]));

        VkImageView uiAttachments[1] = {header.swapchain.views[i]};

        VkFramebufferCreateInfo uiFBCI = {
            VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        uiFBCI.renderPass = header.uiRenderpass.handle;
        uiFBCI.attachmentCount = 1;
        uiFBCI.pAttachments = uiAttachments;
        uiFBCI.width = header.framebufferWidth;
        uiFBCI.height = header.framebufferHeight;
        uiFBCI.layers = 1;

        VULKANSUCCESS(vkCreateFramebuffer(header.device.logicalDevice, &uiFBCI,
                                          header.allocator,
                                          &header.swapchain.framebuffers[i]));
    }
}

b8 recreateSwapchain(rendererBackend* backend) {
    // If already being recreated, do not try again.
    if (header.recreatingSwapchain) {
        FDEBUG("RecreateSwapchain called when already recreating. Booting.");
        return false;
    }

    // Detect if the window is too small to be drawn to
    if (header.framebufferWidth == 0 || header.framebufferHeight == 0) {
        FDEBUG("RecreateSwapchain called when window is zero in at least one "
               "dimension. Booting...");
        return false;
    }

    // Mark as recreating the swapchain
    header.recreatingSwapchain = true;

    vkDeviceWaitIdle(header.device.logicalDevice);

    // TODO: See if I rlly need to do this
    // Clear these out just in case
    for (u32 i = 0; i < header.swapchain.imageCnt; ++i) {
        header.imagesInFlight[i] = 0;
    }

    // Requery support (some of this may not be needed)
    vulkanDeviceQuerySwapchainSupport(header.device.physicalDevice,
                                      header.surface,
                                      &header.device.swapchainSupport);
    vulkanDeviceDetectDepthFormat(&header.device);
    //-----------Requery Support--------

    vulkanSwapchainRecreate(&header, cachedFramebufferWidth,
                            cachedFramebufferHeight, &header.swapchain);
    header.framebufferWidth = cachedFramebufferWidth;
    header.framebufferHeight = cachedFramebufferHeight;
    header.mainRenderpass.renderArea.z = cachedFramebufferWidth;
    header.mainRenderpass.renderArea.w = cachedFramebufferHeight;
    cachedFramebufferWidth = 0;
    cachedFramebufferHeight = 0;

    // Update the framebuffer size gen
    header.framebufferSizeLastGeneration = header.framebufferSizeGeneration;

    // Cleanup swapchain
    for (u32 i = 0; i < header.swapchain.imageCnt; ++i) {
        vulkanCommandBufferFree(&header, header.device.graphicsCommandPool,
                                &header.graphicsCommandBuffers[i]);
    }

    // Destroy framebuffers
    for (u32 i = 0; i < header.swapchain.imageCnt; ++i) {
        vkDestroyFramebuffer(header.device.logicalDevice,
                             header.worldFrameBuffers[i], header.allocator);
        vkDestroyFramebuffer(header.device.logicalDevice,
                             header.swapchain.framebuffers[i],
                             header.allocator);
    }

    header.mainRenderpass.renderArea.x = 0;
    header.mainRenderpass.renderArea.y = 0;
    header.mainRenderpass.renderArea.z = header.framebufferWidth;
    header.mainRenderpass.renderArea.w = header.framebufferHeight;

    regenerateFramebuffers();

    createCommandBuffers(backend);

    // Clear the recreating swapchain flag
    header.recreatingSwapchain = false;

    return true;
}

b8 vulkanCreateTexture(const u8* pixels, texture* outTexture) {
    if (outTexture->hasTransparency) {
        outTexture->flags |= TEXTURE_FLAG_HAS_TRANSPARENCY;
    }

    outTexture->data = (vulkanTextureData*)fallocate(sizeof(vulkanTextureData),
                                                     MEMORY_TAG_TEXTURE);
    vulkanTextureData* data = outTexture->data;

    // TODO: dont assume this
    //  NOTE: Assumes 8 bits per channel.
    VkFormat imgFmt = VK_FORMAT_R8G8B8A8_UNORM;
    if (outTexture->channelCnt == 3) {
        imgFmt = VK_FORMAT_R8G8B8_UNORM;
    }

    // Create a staging buffer and load data into it.
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags memFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    vulkanBuffer staging;
    vulkanBufferCreate(&header,
                       outTexture->width * outTexture->height *
                           outTexture->channelCnt,
                       usage, memFlags, true, true, &staging);
    vulkanBufferLoadData(&header, &staging, 0,
                         outTexture->width * outTexture->height *
                             outTexture->channelCnt,
                         0, pixels);

    // TODO: Make this work for other textures
    //  NOTE: Lots of assumptions here, different texture types will require
    //  different options here.
    vulkanImageCreate(
        &header, VK_IMAGE_TYPE_2D, outTexture->width, outTexture->height,
        imgFmt, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true, VK_IMAGE_ASPECT_COLOR_BIT,
        &data->image);

    vulkanCommandBuffer cb;
    vulkanCommandBufferAllocateAndBeginSingleUse(
        &header, header.device.graphicsCommandPool, &cb);

    vulkanImageTransitionLayout(&header, &cb, &data->image, imgFmt,
                                VK_IMAGE_LAYOUT_UNDEFINED,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    vulkanImageFromBuffer(&header, &data->image, staging.buffer, 0, &cb);

    vulkanImageTransitionLayout(&header, &cb, &data->image, imgFmt,
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vulkanCommandBufferEndSingleUse(&header, header.device.graphicsCommandPool,
                                    &cb, header.device.graphicsQueue);

    vulkanBufferDestroy(&header, &staging);

    VkSamplerCreateInfo samplerInfo = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    // TODO: These filters should be configurable.
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    vkCreateSampler(header.device.logicalDevice, &samplerInfo, header.allocator,
                    &data->sampler);
    outTexture->generation++;
    return true;
}

void vulkanDestroyTexture(texture* texture) {
    vulkanTextureData* data = (vulkanTextureData*)texture->data;
    if (data) {
        vulkanImageDestroy(&header, &data->image);
        vkDestroySampler(header.device.logicalDevice, data->sampler,
                         header.allocator);
        data->sampler = 0;
        ffree(texture->data, sizeof(vulkanTextureData), MEMORY_TAG_TEXTURE);
    }
    fzeroMemory(texture, sizeof(struct texture));
}

void freeDataInfo(vulkanBuffer* buffer, u64 offset, u64 size) {
    if (buffer) {
        vulkanBufferFree(buffer, size, offset);
    }
}

b8 vulkanCreateGeometry(geometry* geometry, u32 vertexStride, u32 vertexCnt,
                        const void* vertices, u32 indexStride, u32 indexCnt,
                        const void* indices) {
    if (!vertexCnt || !vertices) {
        FERROR("vulkanCreateGeometry requires vertex data, and none was "
               "supplied. vertexCnt=%d, vertices=%p",
               vertexCnt, vertices);
        return false;
    }

    // Check if this is a re-upload. If it is, need to free old data afterward.
    b8 firstLoad = geometry->internalID == INVALID_ID;
    vulkanGeometryData old;

    vulkanGeometryData* internalData = 0;
    if (!firstLoad) {
        internalData = &header.geometries[geometry->internalID];

        // Take a copy of the old buffer info.
        old.indexBufferInfo = internalData->indexBufferInfo;
        old.vertexBufferInfo = internalData->vertexBufferInfo;
    } else {
        // Use a hashtable or free list
        for (u32 i = 0; i < VULKAN_MAX_GEOMETRY_COUNT; ++i) {
            if (header.geometries[i].id == INVALID_ID) {
                // Found a free index.
                geometry->internalID = i;
                header.geometries[i].id = i;
                internalData = &header.geometries[i];
                break;
            }
        }
    }
    if (!internalData) {
        FFATAL("vulkanCreateGeometry failed to find a free index for a new "
               "geometry upload. Adjust config to allow for more.");
        return false;
    }

    VkCommandPool pool = header.device.graphicsCommandPool;
    VkQueue queue = header.device.graphicsQueue;

    // Vertex data.
    internalData->vertexBufferInfo.count = vertexCnt;
    internalData->vertexBufferInfo.stride = vertexStride;
    u32 totalSize = vertexCnt * vertexStride;
    if (!uploadDataViaStagingBuffer(
            &header, pool, 0, queue, &header.objectVertexBuffer,
            &internalData->vertexBufferInfo.bufferOffset, totalSize,
            vertices)) {
        FERROR("vulkanCreateGeometry: uploadDataViaStagingBuffer failed.\n");
        return false;
    }

    // Index data, if applicable
    if (indexCnt && indices) {
        internalData->indexBufferInfo.count = indexCnt;
        internalData->indexBufferInfo.stride = indexStride;
        totalSize = indexCnt * indexStride;
        if (!uploadDataViaStagingBuffer(
                &header, pool, 0, queue, &header.objectIndexBuffer,
                &internalData->indexBufferInfo.bufferOffset, totalSize,
                indices)) {
            FERROR(
                "vulkanCreateGeometry: uploadDataViaStagingBuffer failed.\n");
            return false;
        }
    }

    if (internalData->generation == INVALID_ID) {
        internalData->generation = 0;
    } else {
        internalData->generation++;
    }

    if (!firstLoad && old.vertexBufferInfo.stride != 0 &&
        old.vertexBufferInfo.count != 0) {
        // Free vertex data
        freeDataInfo(&header.objectVertexBuffer,
                     old.vertexBufferInfo.bufferOffset,
                     old.vertexBufferInfo.stride * old.vertexBufferInfo.count);

        // Free index data, if applicable
        if (old.indexBufferInfo.stride > 0) {
            freeDataInfo(
                &header.objectIndexBuffer, old.indexBufferInfo.bufferOffset,
                old.indexBufferInfo.stride * old.indexBufferInfo.count);
        }
    }

    return true;
}

void vulkanDestroyGeometry(struct geometry* g) {
    if (g && g->internalID != INVALID_ID) {
        vkDeviceWaitIdle(header.device.logicalDevice);
        // Free the vertex info buffers
        freeDataInfo(
            &header.objectVertexBuffer,
            header.geometries[g->internalID].vertexBufferInfo.bufferOffset,
            header.geometries[g->internalID].vertexBufferInfo.stride *
                header.geometries[g->internalID].vertexBufferInfo.count);
        // If indexes are used free them
        if (header.geometries[g->internalID].indexBufferInfo.stride > 0) {
            freeDataInfo(
                &header.objectVertexBuffer,
                header.geometries[g->internalID].indexBufferInfo.bufferOffset,
                header.geometries[g->internalID].indexBufferInfo.stride *
                    header.geometries[g->internalID].indexBufferInfo.count);
        }

        // Zero the memory and Reset the id/generation so it can be reused
        fzeroMemory(&header.geometries[g->internalID],
                    sizeof(vulkanGeometryData));
        header.geometries[g->internalID].id = INVALID_ID;
        header.geometries[g->internalID].generation = INVALID_ID;
    }
}

// The index of the global descriptor set.
const u32 DESC_SET_INDEX_GLOBAL = 0;
// The index of the instance descriptor set.
const u32 DESC_SET_INDEX_INSTANCE = 1;

// The index of the UBO binding.
const u32 BINDING_INDEX_UBO = 0;

// The index of the image sampler binding.
const u32 BINDING_INDEX_SAMPLER = 1;

b8 createModule(vulkanOverallShader* s, vulkanStageInfo config,
                vulkanShaderStage* stage) {
    // Load binary resource
    resource br;
    resourceLoad(config.fileName, RESOURCE_TYPE_BINARY, &br);

    fzeroMemory(&stage->moduleCreateInfo, sizeof(VkShaderModuleCreateInfo));
    stage->moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    stage->moduleCreateInfo.codeSize = br.dataSize;
    stage->moduleCreateInfo.pCode = (u32*)br.data;

    VULKANSUCCESS(vkCreateShaderModule(header.device.logicalDevice,
                                       &stage->moduleCreateInfo,
                                       header.allocator, &stage->module));
    resourceUnload(&br);

    fzeroMemory(&stage->stageCreateInfo,
                sizeof(VkPipelineShaderStageCreateInfo));
    stage->stageCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stage->stageCreateInfo.stage = config.stage;
    stage->stageCreateInfo.module = stage->module;
    stage->stageCreateInfo.pName = "main";

    return true;
}

b8 vulkanShaderCreate(shader* shader, u8 renderpassID, u8 stageCnt,
                      shaderStage* stages, const char** stageFilenames) {
    if (stageCnt > VULKAN_SHADER_MAX_STAGES) {
        FERROR(
            "Vulkan Shader: StageCnt is more than VULKAN_SHADER_MAX_STAGES\n");
        return false;
    }

    shader->internalData =
        fallocate(sizeof(vulkanOverallShader), MEMORY_TAG_RENDERER);

    // TODO: Make this actually look up the renderpass IDs
    FINFO("Renderpass: %d", renderpassID);

    vulkanOverallShader* vkShader = (vulkanOverallShader*)shader->internalData;

    vkShader->renderpass =
        (renderpassID == 0) ? &header.mainRenderpass : &header.uiRenderpass;

    // TODO: Make this config
    u32 maxDescAllocateCnt = 1024;
    vkShader->config.maxDescSetCnt = maxDescAllocateCnt;

    fzeroMemory(vkShader->config.stages,
                sizeof(vulkanStageInfo) * VULKAN_SHADER_MAX_STAGES);
    vkShader->config.stageCnt = 0;

    VkShaderStageFlagBits stageFlag;
    for (u32 i = 0; i < stageCnt; i++) {
        // Make sure the shader will fit
        if (vkShader->config.stageCnt + 1 > VULKAN_SHADER_MAX_STAGES) {
            FERROR("Shaders can only have a maximum of %d stages",
                   VULKAN_SHADER_MAX_STAGES);
            return false;
        }

        // Convert Fusion shader stages to Vulkans
        switch (stages[i]) {
            case SHADER_STAGE_VERTEX: {
                stageFlag = VK_SHADER_STAGE_VERTEX_BIT;
                break;
            }
            case SHADER_STAGE_FRAGMENT: {
                stageFlag = VK_SHADER_STAGE_FRAGMENT_BIT;
                break;
            }
            case SHADER_STAGE_GEOMETRY: {
                stageFlag = VK_SHADER_STAGE_GEOMETRY_BIT;
                break;
            }
            case SHADER_STAGE_COMPUTE: {
                stageFlag = VK_SHADER_STAGE_COMPUTE_BIT;
                break;
            }
        }

        // Set the stage and increase the cnt
        vkShader->config.stages[i].stage = stageFlag;
        strNCpy(vkShader->config.stages[i].fileName, stageFilenames[i], 255);
        vkShader->config.stageCnt++;
    }

    fzeroMemory(vkShader->config.descriptorSetLayouts,
                sizeof(vulkanDescSetConfig) * 2);

    fzeroMemory(vkShader->config.attributes,
                sizeof(VkVertexInputAttributeDescription) *
                    VULKAN_SHADER_MAX_ATTRIBUTES);

    vkShader->config.poolSize[0] =
        (VkDescriptorPoolSize){VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1024};
    vkShader->config.poolSize[1] =
        (VkDescriptorPoolSize){VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4096};

    vulkanDescSetConfig globalDescSetConfig = {};

    globalDescSetConfig.bindings[BINDING_INDEX_UBO].binding = BINDING_INDEX_UBO;
    globalDescSetConfig.bindings[BINDING_INDEX_UBO].descriptorCount = 1;
    globalDescSetConfig.bindings[BINDING_INDEX_UBO].descriptorType =
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    globalDescSetConfig.bindings[BINDING_INDEX_UBO].stageFlags =
        VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
    globalDescSetConfig.bindCnt++;

    vkShader->config.descriptorSetLayouts[DESC_SET_INDEX_GLOBAL] =
        globalDescSetConfig;
    vkShader->config.descSetCnt++;

    if (shader->hasInstances) {
        vulkanDescSetConfig instanceDescSetConfig = {};

        instanceDescSetConfig.bindings[BINDING_INDEX_UBO].binding =
            BINDING_INDEX_UBO;
        instanceDescSetConfig.bindings[BINDING_INDEX_UBO].descriptorCount = 1;
        instanceDescSetConfig.bindings[BINDING_INDEX_UBO].descriptorType =
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        instanceDescSetConfig.bindings[BINDING_INDEX_UBO].stageFlags =
            VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;
        instanceDescSetConfig.bindCnt++;
        vkShader->config.descriptorSetLayouts[DESC_SET_INDEX_INSTANCE] =
            instanceDescSetConfig;
        vkShader->config.descSetCnt++;
    }

    for (u32 i = 0; i < 1024; i++) {
        vkShader->instanceStates[i].id = INVALID_ID;
    }

    return true;
}

b8 vulkanShaderDestroy(shader* s) {
    if (s && s->internalData) {
        vulkanOverallShader* vkShader = s->internalData;
        if (!vkShader) {
            FERROR("VulkanShaderDestroy needs valid pointer to shader");
            return false;
        }

        for (u32 i = 0; i < vkShader->config.descSetCnt; i++) {
            if (vkShader->descriptorSetLayouts[i]) {
                vkDestroyDescriptorSetLayout(header.device.logicalDevice,
                                             vkShader->descriptorSetLayouts[i],
                                             header.allocator);
                vkShader->descriptorSetLayouts[i] = 0;
            }
        }

        if (vkShader->descriptorPool) {
            vkDestroyDescriptorPool(header.device.logicalDevice,
                                    vkShader->descriptorPool, header.allocator);
        }

        vulkanBufferUnlockMemory(&header, &vkShader->uniformBuffer);
        vkShader->mappedUniformBufferBlock = 0;
        vulkanBufferDestroy(&header, &vkShader->uniformBuffer);

        vulkanPipelineDestroy(&header, &vkShader->pipeline);

        for (u32 i = 0; i < vkShader->config.stageCnt; i++) {
            vkDestroyShaderModule(header.device.logicalDevice,
                                  vkShader->stages[i].module, header.allocator);
        }

        ffree(s->internalData, sizeof(vulkanOverallShader),
              MEMORY_TAG_RENDERER);
        s->internalData = 0;
    }
    return true;
}

b8 vulkanShaderInit(shader* s) {
    vulkanOverallShader* vkShader = (vulkanOverallShader*)s->internalData;

    fzeroMemory(vkShader->stages,
                sizeof(vulkanShaderStage) * VULKAN_SHADER_MAX_STAGES);
    for (u32 i = 0; i < vkShader->config.stageCnt; i++) {
        if (!createModule(vkShader, vkShader->config.stages[i],
                          &vkShader->stages[i])) {
            FERROR("Unable to create shader module.");
            return false;
        }
    }

    // Static lookup table for our types->Vulkan ones.
    static VkFormat* types = 0;
    static VkFormat t[11];
    if (!types) {
        t[SHADER_ATTRIB_TYPE_FLOAT32] = VK_FORMAT_R32_SFLOAT;
        t[SHADER_ATTRIB_TYPE_FLOAT32_2] = VK_FORMAT_R32G32_SFLOAT;
        t[SHADER_ATTRIB_TYPE_FLOAT32_3] = VK_FORMAT_R32G32B32_SFLOAT;
        t[SHADER_ATTRIB_TYPE_FLOAT32_4] = VK_FORMAT_R32G32B32A32_SFLOAT;
        t[SHADER_ATTRIB_TYPE_INT8] = VK_FORMAT_R8_SINT;
        t[SHADER_ATTRIB_TYPE_UINT8] = VK_FORMAT_R8_UINT;
        t[SHADER_ATTRIB_TYPE_INT16] = VK_FORMAT_R16_SINT;
        t[SHADER_ATTRIB_TYPE_UINT16] = VK_FORMAT_R16_UINT;
        t[SHADER_ATTRIB_TYPE_INT32] = VK_FORMAT_R32_SINT;
        t[SHADER_ATTRIB_TYPE_UINT32] = VK_FORMAT_R32_UINT;
        types = t;
    }

    // Process attributes

    // Loop through attri and setup VkVertexInputAttributeDescription
    u32 attriCnt = dinoLength(s->attributes);
    // Keep an ongoing offset for the attributes offset
    u32 offset = 0;
    for (u32 i = 0; i < attriCnt; i++) {
        VkVertexInputAttributeDescription ad;
        ad.offset = offset;
        ad.location = i;
        ad.format = t[s->attributes[i].type];
        ad.binding = 0;
        // Push vkVertex... into shaders config attri list
        vkShader->config.attributes[i] = ad;
        // Inc offset
        offset += s->attributes[i].size;
    }

    // Process Uniforms
    u32 uniCnt = dinoLength(s->uniforms);
    for (u32 i = 0; i < uniCnt; i++) {
        if (s->uniforms[i].type == SHADER_UNIFORM_TYPE_SAMPLER) {
            u32 idx = (s->uniforms[i].scope == SHADER_SCOPE_GLOBAL)
                          ? DESC_SET_INDEX_GLOBAL
                          : DESC_SET_INDEX_INSTANCE;
            vulkanDescSetConfig* c =
                &vkShader->config.descriptorSetLayouts[idx];
            if (c->bindCnt < 2) {
                c->bindings[BINDING_INDEX_SAMPLER].binding =
                    BINDING_INDEX_SAMPLER;
                c->bindings[BINDING_INDEX_SAMPLER].descriptorCount = 1;
                c->bindings[BINDING_INDEX_SAMPLER].descriptorType =
                    VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                c->bindings[BINDING_INDEX_SAMPLER].stageFlags =
                    VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
                c->bindCnt++;
            } else {
                c->bindings[BINDING_INDEX_SAMPLER].descriptorCount++;
            }
        }
    }

    // Create a descriptor pool
    VkDescriptorPoolCreateInfo pci = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pci.poolSizeCount = 2;
    pci.pPoolSizes = vkShader->config.poolSize;
    pci.maxSets = vkShader->config.maxDescSetCnt;
    pci.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    VkResult r =
        vkCreateDescriptorPool(header.device.logicalDevice, &pci,
                               header.allocator, &vkShader->descriptorPool);

    if (!successfullVulkanResult(r)) {
        FERROR("Failed to make descriptor pool.");
        return false;
    }

    fzeroMemory(vkShader->descriptorSetLayouts, vkShader->config.descSetCnt);
    for (u32 i = 0; i < vkShader->config.descSetCnt; i++) {
        VkDescriptorSetLayoutCreateInfo sdci = {
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
        sdci.bindingCount = vkShader->config.descriptorSetLayouts[i].bindCnt;
        sdci.pBindings = vkShader->config.descriptorSetLayouts[i].bindings;
        VkResult r = vkCreateDescriptorSetLayout(
            header.device.logicalDevice, &sdci, header.allocator,
            &vkShader->descriptorSetLayouts[i]);
        if (!successfullVulkanResult(r)) {
            FERROR("Failed to make descriptor set layouts");
            return false;
        }
    }

    // Pipeline creation
    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = (f32)header.framebufferHeight;
    viewport.width = (f32)header.framebufferWidth;
    viewport.height = -(f32)header.framebufferHeight;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // Scissor
    VkRect2D scissor;
    scissor.offset.x = scissor.offset.y = 0;
    scissor.extent.width = header.framebufferWidth;
    scissor.extent.height = header.framebufferHeight;

    VkPipelineShaderStageCreateInfo vsci[VULKAN_SHADER_MAX_STAGES];
    for (u32 i = 0; i < vkShader->config.stageCnt; i++) {
        vsci[i] = vkShader->stages[i].stageCreateInfo;
    }

    vulkanPipelineConfig vpc;
    vpc.renderpass = vkShader->renderpass;
    vpc.descriptorSetLayoutCnt = vkShader->config.descSetCnt;
    vpc.descriptorSetLayouts = vkShader->descriptorSetLayouts;
    vpc.stride = s->attributeStride;
    vpc.attrCnt = attriCnt;
    vpc.viewport = viewport;
    vpc.scissor = scissor;
    vpc.isWireframe = false;
    vpc.shouldDepthTest = true;
    vpc.stages = vsci;
    vpc.stageCnt = vkShader->config.stageCnt;
    vpc.attributes = vkShader->config.attributes;
    vpc.pushConstRangeCnt = s->pushConstRangeCnt;
    vpc.pushConstantRanges = s->pushConstRanges;
    vpc.name = "ShaderPipeline";

    if (!vulkanPipelineCreate(&header, &vpc, &vkShader->pipeline)) {
        FERROR("Failed to make pipeline for shaders");
        return false;
    }

    // Set the UBO alignment reqs
    s->reqUBOAlignment =
        header.device.properties.limits.minUniformBufferOffsetAlignment;
    s->globalUBOStride = getAligned(s->globalUBOSize, s->reqUBOAlignment);
    s->uniformUBOStride = getAligned(s->uniformUBOSize, s->reqUBOAlignment);

    u32 deviceSupportsLocalHost = (header.device.supportsDeviceLocalHost)
                                      ? VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
                                      : 0;

    u64 totalBufferSize =
        s->globalUBOStride + (s->uniformUBOStride * VULKAN_MAX_MATERIAL_COUNT);
    if (!vulkanBufferCreate(&header, totalBufferSize,
                            VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                                deviceSupportsLocalHost,
                            true, true, &vkShader->uniformBuffer)) {
        FERROR("Failed to create Vulkan buffer for object shaders");
        return false;
    }

    if (!vulkanBufferAllocate(&vkShader->uniformBuffer, s->globalUBOStride,
                              &s->globalUBOOffset)) {
        FERROR("Failed to allocate space for uniform buffers");
        return false;
    }

    vkShader->mappedUniformBufferBlock = vulkanBufferLockMemory(
        &header, &vkShader->uniformBuffer, 0, VK_WHOLE_SIZE, 0);

    VkDescriptorSetLayout globalLayouts[3] = {
        vkShader->descriptorSetLayouts[DESC_SET_INDEX_GLOBAL],
        vkShader->descriptorSetLayouts[DESC_SET_INDEX_GLOBAL],
        vkShader->descriptorSetLayouts[DESC_SET_INDEX_GLOBAL]};

    VkDescriptorSetAllocateInfo allocInfo = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocInfo.descriptorPool = vkShader->descriptorPool;
    allocInfo.descriptorSetCount = 3;
    allocInfo.pSetLayouts = globalLayouts;
    VULKANSUCCESS(vkAllocateDescriptorSets(header.device.logicalDevice,
                                           &allocInfo,
                                           vkShader->globalDescriptorSets));

    return true;
}

b8 vulkanShaderUse(shader* shader) {
    vulkanOverallShader* vkShader = (vulkanOverallShader*)shader->internalData;
    vulkanPipelineBind(&header.graphicsCommandBuffers[header.imageIdx],
                       VK_PIPELINE_BIND_POINT_GRAPHICS, &vkShader->pipeline);
    return true;
}

b8 vulkanShaderBindGlobals(shader* shader) {
    shader->boundUboOffset = shader->globalUBOOffset;
    return true;
}

b8 vulkanShaderBindInstance(shader* shader, u32 instanceID) {
    vulkanOverallShader* vkShader = (vulkanOverallShader*)shader->internalData;
    shader->boundInstanceId = instanceID;
    vulkanOverallShaderInstanceState* st =
        &vkShader->instanceStates[instanceID];
    shader->boundUboOffset = st->offset;
    return true;
}

b8 vulkanShaderApplyGlobals(shader* shader) {
    vulkanOverallShader* vkShader = shader->internalData;
    VkCommandBuffer cb = header.graphicsCommandBuffers[header.imageIdx].handle;
    VkDescriptorSet globalDescriptor =
        vkShader->globalDescriptorSets[header.imageIdx];

    VkDescriptorBufferInfo bi;
    bi.buffer = vkShader->uniformBuffer.buffer;
    bi.offset = shader->globalUBOOffset;
    bi.range = shader->globalUBOStride;

    VkWriteDescriptorSet wr = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    wr.dstSet = vkShader->globalDescriptorSets[header.imageIdx];
    wr.dstBinding = 0;
    wr.dstArrayElement = 0;
    wr.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    wr.descriptorCount = 1;
    wr.pBufferInfo = &bi;

    VkWriteDescriptorSet descWrites[2];
    descWrites[0] = wr;

    u32 setBindingCnt =
        vkShader->config.descriptorSetLayouts[DESC_SET_INDEX_GLOBAL].bindCnt;
    if (setBindingCnt > 1) {
        // TODO: This means there are global samplers Support this later
        setBindingCnt = 1;
    }

    vkUpdateDescriptorSets(header.device.logicalDevice, setBindingCnt,
                           descWrites, 0, 0);

    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            vkShader->pipeline.pipelineLayout, 0, 1,
                            &globalDescriptor, 0, 0);
    return true;
}

b8 vulkanShaderApplyInstance(shader* shader) {
    vulkanOverallShader* vkShader = shader->internalData;
    VkCommandBuffer cb = header.graphicsCommandBuffers[header.imageIdx].handle;
    vulkanOverallShaderInstanceState* instanceState =
        &vkShader->instanceStates[shader->boundInstanceId];
    VkDescriptorSet instanceDescSet =
        instanceState->descriptorStates.descriptorSets[header.imageIdx];

    VkWriteDescriptorSet wr[2];
    fzeroMemory(wr, sizeof(VkWriteDescriptorSet) * 2);
    u32 descCnt = 0;
    u32 descIdx = 0;

    u8* instanceUBOGen =
        &(instanceState->descriptorStates.descriptorStates[descIdx]
              .generations[header.imageIdx]);

    // TODO: Do this only if it's needed
    if (*instanceUBOGen == INVALID_ID_U8) {
        VkDescriptorBufferInfo bi;
        bi.buffer = vkShader->uniformBuffer.buffer;
        bi.offset = instanceState->offset;
        bi.range = shader->uniformUBOStride;

        VkWriteDescriptorSet uboDesc = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        uboDesc.dstSet = instanceDescSet;
        uboDesc.dstBinding = descIdx;
        uboDesc.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboDesc.descriptorCount = 1;
        uboDesc.pBufferInfo = &bi;

        wr[descCnt] = uboDesc;
        descCnt++;

        // TODO: Prob update a generation thing here

        *instanceUBOGen = 1;
    }

    descIdx++;

    // If there are samplers
    if (vkShader->config.descriptorSetLayouts[DESC_SET_INDEX_INSTANCE].bindCnt >
        1) {
        u32 totalSamplerCnt =
            vkShader->config.descriptorSetLayouts[DESC_SET_INDEX_INSTANCE]
                .bindings[BINDING_INDEX_SAMPLER]
                .descriptorCount;
        u32 updateSamplerCnt = 0;
        VkDescriptorImageInfo imgInfos[VULKAN_SHADER_MAX_GLOBAL_TEXTURES];
        for (u32 i = 0; i < totalSamplerCnt; i++) {
            texture* t = vkShader->instanceStates[shader->boundInstanceId]
                             .instanceTextures[i];
            vulkanTextureData* d = (vulkanTextureData*)t->data;
            imgInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imgInfos[i].imageView = d->image.view;
            imgInfos[i].sampler = d->sampler;
            updateSamplerCnt++;
        }

        VkWriteDescriptorSet samplerDesc = {
            VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        samplerDesc.dstSet = instanceDescSet;
        samplerDesc.dstBinding = descIdx;
        samplerDesc.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerDesc.descriptorCount = updateSamplerCnt;
        samplerDesc.pImageInfo = imgInfos;

        wr[descCnt] = samplerDesc;
        descCnt++;
    }

    if (descCnt > 0) {
        vkUpdateDescriptorSets(header.device.logicalDevice, descCnt, wr, 0, 0);
    }

    vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            vkShader->pipeline.pipelineLayout, 1, 1,
                            &instanceDescSet, 0, 0);
    return true;
}

b8 vulkanShaderAllocateInstanceStruct(shader* s, u32* outInstanceID) {
    vulkanOverallShader* vkShader = s->internalData;
    *outInstanceID = INVALID_ID;

    // TODO: Do this better
    for (u32 i = 0; i < 1024; i++) {
        if (vkShader->instanceStates[i].id == INVALID_ID) {
            *outInstanceID = i;
            vkShader->instanceStates[i].id = i;
            break;
        }
    }
    if (*outInstanceID == INVALID_ID) {
        FERROR("vulkanShaderAllocateInstanceStruct: Failed to find free "
               "instanceState");
        return false;
    }

    vulkanOverallShaderInstanceState* insState =
        &vkShader->instanceStates[*outInstanceID];
    u32 insTextureCnt =
        vkShader->config.descriptorSetLayouts[DESC_SET_INDEX_INSTANCE]
            .bindings[BINDING_INDEX_SAMPLER]
            .descriptorCount;
    insState->instanceTextures =
        fallocate(sizeof(texture*) * s->instanceTextureCnt, MEMORY_TAG_ARRAY);
    texture* defaultTexture = textureSystemGetDefault();
    // Set all textures to default
    for (u32 i = 0; i < insTextureCnt; i++) {
        insState->instanceTextures[i] = defaultTexture;
    }

    u64 size = s->uniformUBOStride;
    if (!vulkanBufferAllocate(&vkShader->uniformBuffer, size,
                              &insState->offset)) {
        FERROR("vulkanShaderAllocateInstanceStruct: failed to allocate ubo "
               "buffer.");
        return false;
    }

    vulkanDescriptorSetState* set = &insState->descriptorStates;

    u32 bindingCnt =
        vkShader->config.descriptorSetLayouts[DESC_SET_INDEX_INSTANCE].bindCnt;
    for (u32 i = 0; i < bindingCnt; i++) {
        for (u32 j = 0; j < 3; j++) {
            set->descriptorStates[i].generations[j] = INVALID_ID_U8;
            set->descriptorStates[i].ids[j] = INVALID_ID;
        }
    }

    VkDescriptorSetLayout lo[3] = {
        vkShader->descriptorSetLayouts[DESC_SET_INDEX_INSTANCE],
        vkShader->descriptorSetLayouts[DESC_SET_INDEX_INSTANCE],
        vkShader->descriptorSetLayouts[DESC_SET_INDEX_INSTANCE]};

    VkDescriptorSetAllocateInfo ai = {
        VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    ai.descriptorPool = vkShader->descriptorPool;
    ai.descriptorSetCount = 3;
    ai.pSetLayouts = lo;
    VkResult res =
        vkAllocateDescriptorSets(header.device.logicalDevice, &ai,
                                 insState->descriptorStates.descriptorSets);

    if (res != VK_SUCCESS) {
        FERROR("Failed creating descriptor sets");
        return false;
    }
    return true;
}

b8 vulkanShaderFreeInstanceStruct(shader* s, u32 instanceID) {
    vulkanOverallShader* vkShader = s->internalData;
    vulkanOverallShaderInstanceState* insState =
        &vkShader->instanceStates[instanceID];

    vkDeviceWaitIdle(header.device.logicalDevice);

    VkResult res = vkFreeDescriptorSets(
        header.device.logicalDevice, vkShader->descriptorPool, 3,
        insState->descriptorStates.descriptorSets);
    if (res != VK_SUCCESS) {
        FERROR("Error freeing instance struct");
    }

    fzeroMemory(insState->descriptorStates.descriptorStates,
                sizeof(vulkanDescriptorState) * VULKAN_SHADER_MAX_BINDINGS);

    if (insState->instanceTextures) {
        ffree(insState->instanceTextures,
              sizeof(texture*) * s->instanceTextureCnt, MEMORY_TAG_ARRAY);
        insState->instanceTextures = 0;
    }

    vulkanBufferFree(&vkShader->uniformBuffer, s->uniformUBOStride,
                     insState->offset);
    insState->offset = INVALID_ID;
    insState->id = INVALID_ID;
    return true;
}

b8 vulkanShaderUniformSet(shader* s, shaderUniform u, const void* val) {
    vulkanOverallShader* vkShader = s->internalData;
    if (u.type == SHADER_UNIFORM_TYPE_SAMPLER) {
        if (u.scope == SHADER_SCOPE_GLOBAL) {
            s->globalTextures[u.location] = (texture*)val;
        } else {
            vkShader->instanceStates[s->boundInstanceId]
                .instanceTextures[u.location] = (texture*)val;
        }
    } else {
        if (u.scope == SHADER_SCOPE_LOCAL) {
            VkCommandBuffer cb =
                header.graphicsCommandBuffers[header.imageIdx].handle;
            vkCmdPushConstants(cb, vkShader->pipeline.pipelineLayout,
                               VK_SHADER_STAGE_VERTEX_BIT |
                                   VK_SHADER_STAGE_FRAGMENT_BIT,
                               u.offset, u.size, val);
            return true;
        }

        u64 uniformMappedPt = (u64)vkShader->mappedUniformBufferBlock +
                              s->boundUboOffset + u.offset;
        fcopyMemory((void*)uniformMappedPt, val, u.size);
    }
    return true;
}
