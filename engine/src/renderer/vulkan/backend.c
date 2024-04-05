#include "backend.h"
#include "vulkanHeader.h"
#include "vulkanPlatform.h"
#include "vulkanDevice.h"
#include "vulkanSwapchain.h"
#include "renderpass.h"
#include "commandBuffer.h"
#include "utils.h"
#include "vulkanBuffers.h"
#include "vulkanImage.h"

#include "core/logger.h"
#include "core/fstring.h"
#include "core/fmemory.h"
#include "core/application.h"

#include "shaders/vulkanMaterialShader.h"
#include "shaders/vulkanUIShader.h"

#include "helpers/dinoArray.h"

#include "math/matrixMath.h"

#include "platform/platform.h"
#include "resources/resourcesTypes.h"
#include "systems/textureSystem.h"
#include "systems/materialSystem.h"
#include "systems/geometrySystem.h"
#include <vulkan/vulkan_core.h>

static vulkanHeader header;
static u32 cachedFramebufferWidth = 0;
static u32 cachedFramebufferHeight = 0;

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void* userData);

i32 findMemoryIdx(u32 typeFilter, u32 propertyFlags);

void createCommandBuffers(rendererBackend* backend);
void regenerateFramebuffers();

b8 recreateSwapchain(rendererBackend* backend);

void uploadDataViaStagingBuffer(vulkanHeader* header, VkCommandPool pool, VkFence fence, VkQueue queue, vulkanBuffer* buffer, u64 offset, u64 size, const void* data) {
    vulkanBuffer vb;
    vulkanBufferCreate(header, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true, &vb);

    vulkanBufferLoadData(header, &vb, 0, size, 0, data);
    
    //Vulkan copy buffers
    vulkanBufferCopyTo(header, pool, fence, queue, vb.buffer, 0, buffer->buffer, offset, size);

    vulkanBufferDestroy(header, &vb);
}

b8 createBuffers(vulkanHeader* header) {
    VkMemoryPropertyFlagBits memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    const u64 vertexBufferSize = sizeof(vertex3D) * 1024 * 1024;
    if (!vulkanBufferCreate(
            header,
            vertexBufferSize,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            memoryPropertyFlags,
            true,
            &header->objectVertexBuffer)) {
        FERROR("Error creating vertex buffer.");
        return false;
    }
    header->geometryVertexOffset = 0;

    const u64 indexBufferSize = sizeof(u32) * 1024 * 1024;
    if (!vulkanBufferCreate(
            header,
            indexBufferSize,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            memoryPropertyFlags,
            true,
            &header->objectIndexBuffer)) {
        FERROR("Error creating vertex buffer.");
        return false;
    }
    header->geometryIndexOffset = 0;

    return true;
}

b8 vulkanInit(struct rendererBackend* backend, const char* appName){
    
    header.findMemoryIdx = findMemoryIdx;
    //TODO: Custom allocator
    header.allocator = 0;

    appGetFramebufferSize(&cachedFramebufferWidth, &cachedFramebufferHeight);
    header.framebufferWidth = (cachedFramebufferWidth != 0) ? cachedFramebufferWidth : 800;
    header.framebufferHeight = (cachedFramebufferHeight != 0) ? cachedFramebufferHeight : 600;
    cachedFramebufferWidth = 0;
    cachedFramebufferHeight = 0;

    VkApplicationInfo appInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
    appInfo.pApplicationName = appName;
    appInfo.applicationVersion = VK_MAKE_VERSION(1,0,0);
    appInfo.pEngineName = "Fusion Game Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1,0,0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo instanceInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    instanceInfo.pApplicationInfo = &appInfo;

    //--------Vulkan Extensions--------
    const char** requiredExts = dinoCreate(const char**);
    dinoPush(requiredExts, &VK_KHR_SURFACE_EXTENSION_NAME);
    platformGetRequiredExts(&requiredExts);

    //Debug vulkan extensions
    #if defined(_DEBUG)
    dinoPush(requiredExts, &VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    FDEBUG("Required extensions:");
    u32 length = dinoLength(requiredExts);
    for (u32 i = 0; i < length; ++i) {
        FDEBUG(requiredExts[i]);
    }
    #endif

    //Validation Layers
    const char** requiredValLayers = 0;
    u32 requiredValLayersCnt = 0;

#if defined(_DEBUG)
    requiredValLayers = dinoCreate(const char**);
    dinoPush(requiredValLayers, &"VK_LAYER_KHRONOS_validation");
    requiredValLayersCnt = dinoLength(requiredValLayers);

    // Obtain a list of available validation layers
    u32 availLayerCnt = 0;
    VULKANSUCCESS(vkEnumerateInstanceLayerProperties(&availLayerCnt, 0));
    VkLayerProperties* availLayers = dinoCreateReserve(availLayerCnt, VkLayerProperties);
    VULKANSUCCESS(vkEnumerateInstanceLayerProperties(&availLayerCnt, availLayers));

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
            FFATAL("Required validation layer is missing: %s", requiredValLayers[i]);
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

    //TODO: Create custom allocater
    VULKANSUCCESS(vkCreateInstance(&instanceInfo,header.allocator,&header.instance));
    FINFO("Vulkan Instance Created")

    //Clear up the dinos
    dinoDestroy(requiredExts);
#if defined(_DEBUG)
    dinoDestroy(requiredValLayers);
#endif

    // Debugger
#if defined(_DEBUG)
    FDEBUG("Creating Vulkan debugger...");
    u32 log_severity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;  //|
                                                                      //    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
    debugCreateInfo.messageSeverity = log_severity;
    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
    debugCreateInfo.pfnUserCallback = vk_debugCallback;

    PFN_vkCreateDebugUtilsMessengerEXT func =
        (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(header.instance, "vkCreateDebugUtilsMessengerEXT");
    FASSERT_MSG(func, "Failed to create debug messenger!");
    VULKANSUCCESS(func(header.instance, &debugCreateInfo, header.allocator, &header.debugMessenger));
    FDEBUG("Vulkan debugger created.");
#endif

    //Create surface
    if (!platformCreateVulkanSurface(&header)){
        return false;
    }
    FDEBUG("Vulkan surface created");

    //Create Device
    if (!getVulkanDevice(&header)){
        FFATAL("Couldn't get physical device");
        return false;
    }

    //Create Swapchain
    vulkanSwapchainCreate(
        &header,
        header.framebufferWidth,
        header.framebufferHeight,
        &header.swapchain);

    //------------------Create Renderpasses
    // World Renderpass
    vulkanRenderpassCreate(
        &header,
        &header.mainRenderpass,
        (vector4){0, 0, header.framebufferWidth, header.framebufferHeight},
        (vector4){0.0f, 0.0f, 0.2f, 1.0f},
        1.0f,
        0,
        RENDERPASS_CLEAR_COLOR_BUFFER_FLAG | RENDERPASS_CLEAR_DEPTH_BUFFER_FLAG | RENDERPASS_CLEAR_STENCIL_BUFFER_FLAG,
        false, true);
    // UI Renderpass
    vulkanRenderpassCreate(
        &header,
        &header.uiRenderpass,
        (vector4){0, 0, header.framebufferWidth, header.framebufferHeight},
        (vector4){0.0f, 0.0f, 0.2f, 1.0f},
        1.0f,
        0,
        RENDERPASS_CLEAR_NONE_FLAG,
        true, false);

    //Swapchain framebuffers
    regenerateFramebuffers();
    
    //Create Command Buffers
    createCommandBuffers(backend);

    //Create Sync Objects
    header.imageAvailSemaphores = dinoCreateReserve(header.swapchain.maxNumOfFramesInFlight, VkSemaphore);
    header.queueCompleteSemaphores = dinoCreateReserve(header.swapchain.maxNumOfFramesInFlight, VkSemaphore);

    for (u8 i = 0; i < header.swapchain.maxNumOfFramesInFlight; ++i){
        VkSemaphoreCreateInfo semaphoreCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        vkCreateSemaphore(header.device.logicalDevice, &semaphoreCreateInfo, header.allocator, &header.imageAvailSemaphores[i]);
        vkCreateSemaphore(header.device.logicalDevice, &semaphoreCreateInfo, header.allocator, &header.queueCompleteSemaphores[i]);

        //Create the fence in a signaled state, indicating that the first frame has already been "rendered".
        //This will prevent the app from waiting indefinitely for the first frame to render since it
        //Cannot be rendered until a frame is "rendered" before it
        VkFenceCreateInfo fenceCI = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        fenceCI.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        VULKANSUCCESS(vkCreateFence(header.device.logicalDevice, &fenceCI, header.allocator, &header.inFlightFences[i]));
    }

    // In flight fences should not yet exist at this point, so clear the list. These are stored in pointers
    // because the initial state should be 0, and will be 0 when not in use. Acutal fences are not owned
    // by this list.
    for (u32 i = 0; i < header.swapchain.imageCnt; ++i) {
        header.imagesInFlight[i] = 0;
    }

    if (!vulkanMaterialShaderCreate(&header, &header.materialShader)){
        FERROR("Failed to create shaders");
    }
    if (!vulkanUIShaderCreate(&header, &header.uiShader)){
        FERROR("Failed to create shaders");
    }
    FINFO("Created shaders");

    //Create buffers
    createBuffers(&header);

    // Mark all geometries as invalid
    for (u32 i = 0; i < VULKAN_MAX_GEOMETRY_COUNT; ++i) {
        header.geometries[i].id = INVALID_ID;
    }

    FINFO("Vulkan Rendering subsystem inited");
    return true;
}

void vulkanShutdown(struct rendererBackend* backend){
    vkDeviceWaitIdle(header.device.logicalDevice);

    // Destroy in the opposite order of creation.

    vulkanBufferDestroy(&header, &header.objectVertexBuffer);
    vulkanBufferDestroy(&header, &header.objectIndexBuffer);

    vulkanUIShaderDestroy(&header, &header.uiShader);
    vulkanMaterialShaderDestroy(&header, &header.materialShader);

    //Sync Objects
    for (u8 i = 0; i < header.swapchain.maxNumOfFramesInFlight; ++i){
        if (header.imageAvailSemaphores[i]){
            vkDestroySemaphore(header.device.logicalDevice, header.imageAvailSemaphores[i], header.allocator);
            header.imageAvailSemaphores[i] = 0;
        }
        if (header.queueCompleteSemaphores[i]){
            vkDestroySemaphore(header.device.logicalDevice, header.queueCompleteSemaphores[i], header.allocator);
            header.queueCompleteSemaphores[i] = 0;
        }
        vkDestroyFence(header.device.logicalDevice, header.inFlightFences[i], header.allocator);
    }
    dinoDestroy(header.imageAvailSemaphores);
    header.imageAvailSemaphores = 0;

    dinoDestroy(header.queueCompleteSemaphores);
    header.queueCompleteSemaphores = 0;

    //Command Buffers
    for (u32 i = 0; i < header.swapchain.imageCnt; ++i){
        if (header.graphicsCommandBuffers[i].handle){
            vulkanCommandBufferFree(&header, header.device.graphicsCommandPool, &header.graphicsCommandBuffers[i]);
            header.graphicsCommandBuffers[i].handle = 0;
        }
    }
    dinoDestroy(header.graphicsCommandBuffers);
    header.graphicsCommandBuffers = 0;

    //Destroy Framebuffers
    for (u32 i = 0; i < header.swapchain.imageCnt; ++i){
        vkDestroyFramebuffer(header.device.logicalDevice, header.worldFrameBuffers[i], header.allocator);
        vkDestroyFramebuffer(header.device.logicalDevice, header.swapchain.framebuffers[i], header.allocator);
    }

    //Renderpass
    vulkanRenderpassDestroy(&header, &header.uiRenderpass);
    vulkanRenderpassDestroy(&header, &header.mainRenderpass);

    //Swapchain
    vulkanSwapchainDestroy(&header, &header.swapchain);

    //Device
    vulkanDeviceDestroy(&header);

    if (header.surface){
        vkDestroySurfaceKHR(header.instance,header.surface,header.allocator);
        header.surface = 0;
    }

    #if defined(_DEBUG)
    FDEBUG("Destroying Vulkan debugger...");
    if (header.debugMessenger) {
        PFN_vkDestroyDebugUtilsMessengerEXT func =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(header.instance, "vkDestroyDebugUtilsMessengerEXT");
        func(header.instance, header.debugMessenger, header.allocator);
    }
    #endif
    FDEBUG("Destroying Vulkan instance...");
    vkDestroyInstance(header.instance,header.allocator);
}

void vulkanResized(struct rendererBackend* backend, u16 width, u16 height){
    //Update the "framebuffer size generation", a counter which indicates when the
    //framebuffer size has been updated
    cachedFramebufferWidth = width;
    cachedFramebufferHeight = height;
    header.framebufferSizeGeneration++;

    FINFO("Vulkan renderer backend->resized: w/h/gen: %i/%i/%llu",width,height,header.framebufferSizeGeneration);
}

void vulkanUpdateGlobalState(mat4 projection, mat4 view, vector3 viewPos, vector4 ambientColor, i32 mode){
    vulkanMaterialShaderUse(&header, &header.materialShader);

    header.materialShader.globalUbo.proj = projection;
    header.materialShader.globalUbo.view = view;

    vulkanMaterialShaderUpdateGlobalState(&header, &header.materialShader);
}

void vulkanUpdateUIState(mat4 projection, mat4 view, i32 mode){
    vulkanUIShaderUse(&header, &header.uiShader);

    header.uiShader.globalUbo.proj = projection;
    header.uiShader.globalUbo.view = view;

    vulkanUIShaderUpdateGlobalState(&header, &header.uiShader);
}

void vulkanDrawGeometry(geometryRenderData data){
    vulkanGeometryData* bd = &header.geometries[data.geometry->internalID];
    vulkanCommandBuffer* cb = &header.graphicsCommandBuffers[header.imageIdx];
    //vulkanMaterialShaderUse(&header, &header.materialShader);

    material* m = 0;
    if (data.geometry->material){
        m = data.geometry->material;
    }else{
        m = materialSystemGetDefault();
    }

    switch (m->type){
        case MATERIAL_TYPE_WORLD: {
            vulkanMaterialShaderSetModel(&header, &header.materialShader, data.model);
            vulkanMaterialShaderApplyMaterial(&header, &header.materialShader, m);
            break;
        }
        case MATERIAL_TYPE_UI: {
            vulkanUIShaderSetModel(&header, &header.uiShader, data.model);
            vulkanUIShaderApplyMaterial(&header, &header.uiShader, m);
            break;
        }
    }


    // Bind vertex buffer at offset.
    VkDeviceSize offsets[1] = {bd->vertexBufferInfo.bufferOffset};
    vkCmdBindVertexBuffers(cb->handle, 0, 1, &header.objectVertexBuffer.buffer, (VkDeviceSize*)offsets);

    if (bd->indexBufferInfo.count > 0){
        // Bind index buffer at offset.
        vkCmdBindIndexBuffer(cb->handle, header.objectIndexBuffer.buffer, bd->indexBufferInfo.bufferOffset, VK_INDEX_TYPE_UINT32);
        // Issue the draw.
        vkCmdDrawIndexed(cb->handle, bd->indexBufferInfo.count, 1, 0, 0, 0);
    }else{
        vkCmdDraw(cb->handle, bd->vertexBufferInfo.count, 1, 0, 0);
    }
}

b8 vulkanBeginFrame(struct rendererBackend* backend, f32 deltaTime){
    header.deltaTime = deltaTime;
    vulkanDevice* dev = &header.device;
    if (header.recreatingSwapchain){
        VkResult res = vkDeviceWaitIdle(dev->logicalDevice);
        if (!successfullVulkanResult(res)){
            FERROR("Vulkan renderer begin frame vkDeviceWaitIdle failed: %s", vulkanResultStr(res, true));
            return false;
        }
        FINFO("Recreating swapchain...");
        return false;
    }

    //Check if the window has been resized
    if (header.framebufferSizeGeneration != header.framebufferSizeLastGeneration){
        VkResult res = vkDeviceWaitIdle(dev->logicalDevice);
        if (!successfullVulkanResult(res)){
            FERROR("Vulkan renderer begin frame vkDeviceWaitIdle(2) failed: %s", vulkanResultStr(res, true));
            return false;
        }

        //If the swapchain recreation failed (cuz the window was minimized, or paused in some way)
        //boot out before unsetting the flag
        if (!recreateSwapchain(backend)){
            return false;
        }

        FINFO("Resized, booting");
        return false;
    }

    VkResult result = vkWaitForFences(header.device.logicalDevice, 1, &header.inFlightFences[header.currentFrame], true, UINT64_MAX);
    if (!successfullVulkanResult(result)){
        FERROR("In flight fence wait failed. %s", vulkanResultStr(result, true));
        return false;
    }

    if (!vulkanSwapchainGetNextImgIdx(&header, &header.swapchain, UINT64_MAX, header.imageAvailSemaphores[header.currentFrame], 0, &header.imageIdx)){
        return false;
    }

    //Begin recording commands
    vulkanCommandBuffer* cb = &header.graphicsCommandBuffers[header.imageIdx];
    vulkanCommandBufferReset(cb);
    vulkanCommandBufferBegin(cb, false, false, false);

    //Dynamic state
    //Start at the bottom left corner
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

b8 vulkanEndFrame(struct rendererBackend* backend, f32 deltaTime){
    vulkanCommandBuffer* cb = &header.graphicsCommandBuffers[header.imageIdx];

    vulkanCommandBufferEnd(cb);

    if (header.imagesInFlight[header.imageIdx] != VK_NULL_HANDLE){
        VkResult result = vkWaitForFences(header.device.logicalDevice, 1, &header.inFlightFences[header.currentFrame], true, UINT64_MAX);
        if (!successfullVulkanResult(result)){
            FFATAL("In flight fence wait failed. %s", vulkanResultStr(result, true));
            return false;
        }
    }

    //Mark the image fence as in-use by this frame
    header.imagesInFlight[header.imageIdx] = &header.inFlightFences[header.currentFrame];

    //Reset the fence for use on the next frame
    VULKANSUCCESS(vkResetFences(header.device.logicalDevice, 1, &header.inFlightFences[header.currentFrame]));

    VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    //Command buffer(s) to be executed.
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cb->handle;
    //The semaphore(s) to be signaled when the queue is complete
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &header.queueCompleteSemaphores[header.currentFrame];
    //Wait semaphore ensures that the operation cannot begin until the image is available.
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &header.imageAvailSemaphores[header.currentFrame];

    //Each semaphore waits to ensure only one is shown (presented) at a time
    VkPipelineStageFlags flags[1] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.pWaitDstStageMask = flags;

    VkResult res = vkQueueSubmit(header.device.graphicsQueue, 1, &submitInfo, header.inFlightFences[header.currentFrame]);
    if (!successfullVulkanResult(res)){
        FERROR("vkQueueSubmit failed with a result of: %s", vulkanResultStr(res, true));
        return false;
    }

    vulkanCommandBufferUpdateSubmitted(cb);
    //-----------------End queue submission----------------

    //Give the image back to the swapchain
    vulkanSwapchainPresent(&header, &header.swapchain, header.device.graphicsQueue,
        header.device.presentQueue, header.queueCompleteSemaphores[header.currentFrame], header.imageIdx);

    return true;
}

b8 vulkanBeginRenderpass(struct rendererBackend* backend, u8 renderpassID){
    VkFramebuffer fb = 0;
    vulkanRenderpass* rp = 0;
    vulkanCommandBuffer* cb = &header.graphicsCommandBuffers[header.imageIdx];

    switch(renderpassID){
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
            FWARN("Begining renderpass of unknown id failed. %#02x", renderpassID);
            return false;
        }
    }

    vulkanRenderpassBegin(cb, rp, fb);

    switch (renderpassID) {
        case BUILTIN_RENDERPASS_WORLD: {
            vulkanMaterialShaderUse(&header, &header.materialShader);
            break;
        }
        case BUILTIN_RENDERPASS_UI: {
            vulkanUIShaderUse(&header, &header.uiShader);
            break;
        }
    }

    return true;
}

b8 vulkanEndRenderpass(struct rendererBackend* backend, u8 renderpassID){
    vulkanRenderpass* rp = 0;
    vulkanCommandBuffer* cb = &header.graphicsCommandBuffers[header.imageIdx];

    switch(renderpassID){
        case BUILTIN_RENDERPASS_WORLD: {
            rp = &header.mainRenderpass;
            break;
        }
        case BUILTIN_RENDERPASS_UI: {
            rp = &header.uiRenderpass;
            break;
        }
        default: {
            FWARN("Ending renderpass of unknown id failed. %#02x", renderpassID);
            return false;
        }
    }

    vulkanRenderpassEnd(cb, rp);
    return true;
}

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
    void* userData) {
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

i32 findMemoryIdx(u32 typeFilter, u32 propertyFlags){
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(header.device.physicalDevice, &memoryProperties);

    for (u32 i = 0; i < memoryProperties.memoryTypeCount; ++i) {
        // Check each memory type to see if its bit is set to 1.
        if (typeFilter & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags) {
            return i;
        }
    }

    FWARN("Unable to find suitable memory type!");
    return -1;
}

void createCommandBuffers(rendererBackend* backend){
    if (!header.graphicsCommandBuffers){
        header.graphicsCommandBuffers = dinoCreateReserve(header.swapchain.imageCnt, vulkanCommandBuffer);
        for (u32 i = 0; i < header.swapchain.imageCnt; ++i){
            fzeroMemory(&header.graphicsCommandBuffers[i],sizeof(vulkanCommandBuffer));
        }
    }

    for (u32 i = 0; i < header.swapchain.imageCnt; ++i){
        if (header.graphicsCommandBuffers[i].handle){
            vulkanCommandBufferFree(&header, header.device.graphicsCommandPool, &header.graphicsCommandBuffers[i]);
        }
        fzeroMemory(&header.graphicsCommandBuffers[i],sizeof(vulkanCommandBuffer));
        vulkanCommandBufferAllocate(&header, header.device.graphicsCommandPool, true, &header.graphicsCommandBuffers[i]);
    }

    FINFO("Vulkan command buffers created.");
}

void regenerateFramebuffers(){
    u32 cnt = header.swapchain.imageCnt;
    for (u32 i = 0; i < cnt; ++i){
        VkImageView worldAttachments[2] = {
            header.swapchain.views[i],
            header.swapchain.depthAttachment.view
        };

        VkFramebufferCreateInfo worldFBCI = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        worldFBCI.renderPass = header.mainRenderpass.handle;
        worldFBCI.attachmentCount = 2;
        worldFBCI.pAttachments = worldAttachments;
        worldFBCI.width = header.framebufferWidth;
        worldFBCI.height = header.framebufferHeight;
        worldFBCI.layers = 1;

        VULKANSUCCESS(vkCreateFramebuffer(header.device.logicalDevice, &worldFBCI, header.allocator, &header.worldFrameBuffers[i]));
        
        VkImageView uiAttachments[1] = {header.swapchain.views[i]};

        VkFramebufferCreateInfo uiFBCI = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
        uiFBCI.renderPass = header.uiRenderpass.handle;
        uiFBCI.attachmentCount = 1;
        uiFBCI.pAttachments = uiAttachments;
        uiFBCI.width = header.framebufferWidth;
        uiFBCI.height = header.framebufferHeight;
        uiFBCI.layers = 1;

        VULKANSUCCESS(vkCreateFramebuffer(header.device.logicalDevice, &uiFBCI, header.allocator, &header.swapchain.framebuffers[i]));
    }
}

b8 recreateSwapchain(rendererBackend* backend){
    //If already being recreated, do not try again.
    if (header.recreatingSwapchain){
        FDEBUG("RecreateSwapchain called when already recreating. Booting.");
        return false;
    }

    //Detect if the window is too small to be drawn to
    if (header.framebufferWidth == 0 || header.framebufferHeight == 0){
        FDEBUG("RecreateSwapchain called when window is zero in at least one dimension. Booting...");
        return false;
    }

    //Mark as recreating the swapchain
    header.recreatingSwapchain = true;

    vkDeviceWaitIdle(header.device.logicalDevice);

    //TODO: See if I rlly need to do this
    //Clear these out just in case
    for (u32 i = 0; i < header.swapchain.imageCnt; ++i){
        header.imagesInFlight[i] = 0;
    }

    //Requery support (some of this may not be needed)
    vulkanDeviceQuerySwapchainSupport(header.device.physicalDevice, header.surface, &header.device.swapchainSupport);
    vulkanDeviceDetectDepthFormat(&header.device);
    //-----------Requery Support--------

    vulkanSwapchainRecreate(&header, cachedFramebufferWidth, cachedFramebufferHeight, &header.swapchain);
    header.framebufferWidth = cachedFramebufferWidth;
    header.framebufferHeight = cachedFramebufferHeight;
    header.mainRenderpass.renderArea.z = cachedFramebufferWidth;
    header.mainRenderpass.renderArea.w = cachedFramebufferHeight;
    cachedFramebufferWidth = 0;
    cachedFramebufferHeight = 0;

    //Update the framebuffer size gen
    header.framebufferSizeLastGeneration = header.framebufferSizeGeneration;

    //Cleanup swapchain
    for (u32 i = 0; i < header.swapchain.imageCnt; ++i){
        vulkanCommandBufferFree(&header, header.device.graphicsCommandPool, &header.graphicsCommandBuffers[i]);
    }

    //Destroy framebuffers
    for (u32 i = 0; i < header.swapchain.imageCnt; ++i){
        vkDestroyFramebuffer(header.device.logicalDevice, header.worldFrameBuffers[i], header.allocator);
        vkDestroyFramebuffer(header.device.logicalDevice, header.swapchain.framebuffers[i], header.allocator);
    }

    header.mainRenderpass.renderArea.x = 0;
    header.mainRenderpass.renderArea.y = 0;
    header.mainRenderpass.renderArea.z = header.framebufferWidth;
    header.mainRenderpass.renderArea.w = header.framebufferHeight;

    regenerateFramebuffers();

    createCommandBuffers(backend);

    //Clear the recreating swapchain flag
    header.recreatingSwapchain = false;

    return true;
}

b8 vulkanCreateTexture(const u8* pixels, texture* outTexture){
    if (outTexture->hasTransparency){
        outTexture->flags |= TEXTURE_FLAG_HAS_TRANSPARENCY;
    }

    outTexture->data = (vulkanTextureData*)fallocate(sizeof(vulkanTextureData), MEMORY_TAG_TEXTURE);
    vulkanTextureData* data = outTexture->data;

    //TODO: dont assume this
    // NOTE: Assumes 8 bits per channel.
    VkFormat imgFmt = VK_FORMAT_R8G8B8A8_UNORM;
    if (outTexture->channelCnt == 3){
        imgFmt = VK_FORMAT_R8G8B8_UNORM;
    }

    // Create a staging buffer and load data into it.
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags memFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    vulkanBuffer staging;
    vulkanBufferCreate(&header, outTexture->width * outTexture->height * outTexture->channelCnt, usage, memFlags, true, &staging);
    vulkanBufferLoadData(&header, &staging, 0, outTexture->width * outTexture->height * outTexture->channelCnt, 0, pixels);
    
    //TODO: Make this work for other textures
    // NOTE: Lots of assumptions here, different texture types will require
    // different options here.
    vulkanImageCreate(&header, VK_IMAGE_TYPE_2D, outTexture->width, outTexture->height, imgFmt, VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true, VK_IMAGE_ASPECT_COLOR_BIT, &data->image);

    vulkanCommandBuffer cb;
    vulkanCommandBufferAllocateAndBeginSingleUse(&header, header.device.graphicsCommandPool, &cb);

    vulkanImageTransitionLayout(&header, &cb, &data->image, imgFmt, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    vulkanImageFromBuffer(&header, &data->image, staging.buffer, 0, &cb);

    vulkanImageTransitionLayout(&header, &cb, &data->image, imgFmt, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vulkanCommandBufferEndSingleUse(&header, header.device.graphicsCommandPool, &cb, header.device.graphicsQueue);

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

    vkCreateSampler(header.device.logicalDevice, &samplerInfo, header.allocator, &data->sampler);
    outTexture->generation++;
    return true;
}

void vulkanDestroyTexture(texture* texture){
    vulkanTextureData* data = (vulkanTextureData*)texture->data;
    if (data){
        vulkanImageDestroy(&header, &data->image);
        vkDestroySampler(header.device.logicalDevice, data->sampler, header.allocator);
        data->sampler = 0;
        ffree(texture->data, sizeof(vulkanTextureData), MEMORY_TAG_TEXTURE);
    }
    fzeroMemory(texture, sizeof(struct texture));
}

b8 vulkanCreateMaterial(material* m){
    if (m){
        switch (m->type){
            case MATERIAL_TYPE_WORLD: {
                if (!vulkanMaterialShaderResourceAcquire(&header, &header.materialShader, m)){
                    FERROR("Couln't load shader Material resources.");
                    return false;
                }
            }
            case MATERIAL_TYPE_UI: {
                if (!vulkanUIShaderResourceAcquire(&header, &header.uiShader, m)){
                    FERROR("Couln't load shader UI resources.");
                    return false;
                }
            }
        }
        return true;
    }
    return false;
}

void vulkanDestroyMaterial(material* m){
    if (m){
        if (m->id != INVALID_ID){
            switch (m->type){
                case MATERIAL_TYPE_WORLD: {
                    vulkanMaterialShaderResourceRelease(&header, &header.materialShader, m);
                }
                case MATERIAL_TYPE_UI: {
                    vulkanUIShaderResourceRelease(&header, &header.uiShader, m);
                }
            }
        }else{
            FWARN("vulkanDestroyMaterial called with id=INVALID_ID. Nothing was done. %s", m->name);
        }
    }
    FWARN("vulkanDestroyMaterial called without a material (null). Nothing was done. %s", m->name);
}

void freeDataInfo(vulkanBuffer* buffer, u64 offset, u64 size) {
    // TODO: Free this in the buffer.
    // TODO: update free list with this range being free.
}

b8 vulkanCreateGeometry(geometry* geometry, u32 vertexStride, u32 vertexCnt, const void* vertices, u32 indexStride, u32 indexCnt, const void* indices){
    if (!vertexCnt || !vertices) {
        FERROR("vulkanCreateGeometry requires vertex data, and none was supplied. vertexCnt=%d, vertices=%p", vertexCnt, vertices);
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
        FFATAL("vulkanCreateGeometry failed to find a free index for a new geometry upload. Adjust config to allow for more.");
        return false;
    }

    VkCommandPool pool = header.device.graphicsCommandPool;
    VkQueue queue = header.device.graphicsQueue;

    // Vertex data.
    internalData->vertexBufferInfo.bufferOffset = header.geometryVertexOffset;
    internalData->vertexBufferInfo.count = vertexCnt;
    internalData->vertexBufferInfo.stride = vertexStride;
    u32 totalSize = vertexCnt * vertexStride;
    uploadDataViaStagingBuffer(&header, pool, 0, queue, &header.objectVertexBuffer, internalData->vertexBufferInfo.bufferOffset, totalSize, vertices);
    // TODO: should maintain a free list instead of this.
    header.geometryVertexOffset += totalSize;

    // Index data, if applicable
    if (indexCnt && indices) {
        internalData->indexBufferInfo.bufferOffset = header.geometryIndexOffset;
        internalData->indexBufferInfo.count = indexCnt;
        internalData->indexBufferInfo.stride = indexStride;
        totalSize = indexCnt * indexStride;
        uploadDataViaStagingBuffer(&header, pool, 0, queue, &header.objectIndexBuffer, internalData->indexBufferInfo.bufferOffset, totalSize, indices);
        // TODO: should maintain a free list instead of this.
        header.geometryIndexOffset += totalSize;
    }

    if (internalData->generation == INVALID_ID) {
        internalData->generation = 0;
    } else {
        internalData->generation++;
    }

    if (!firstLoad) {
        // Free vertex data
        freeDataInfo(&header.objectVertexBuffer, old.vertexBufferInfo.bufferOffset, old.vertexBufferInfo.stride * old.vertexBufferInfo.count);

        // Free index data, if applicable
        if (old.indexBufferInfo.stride > 0) {
            freeDataInfo(&header.objectIndexBuffer, old.indexBufferInfo.bufferOffset, old.indexBufferInfo.stride * old.indexBufferInfo.count);
        }
    }

    return true;
}

void vulkanDestroyGeometry(struct geometry* g){
    if (g && g->internalID != INVALID_ID){
        vkDeviceWaitIdle(header.device.logicalDevice);
        // Free the vertex info buffers
        freeDataInfo(&header.objectVertexBuffer, header.geometries[g->internalID].vertexBufferInfo.bufferOffset, header.geometries[g->internalID].vertexBufferInfo.stride * header.geometries[g->internalID].vertexBufferInfo.count);
        // If indexes are used free them
        if (header.geometries[g->internalID].indexBufferInfo.stride > 0){
            freeDataInfo(&header.objectVertexBuffer, header.geometries[g->internalID].indexBufferInfo.bufferOffset, header.geometries[g->internalID].indexBufferInfo.stride * header.geometries[g->internalID].indexBufferInfo.count);
        }

        // Zero the memory and Reset the id/generation so it can be reused
        fzeroMemory(&header.geometries[g->internalID], sizeof(vulkanGeometryData));
        header.geometries[g->internalID].id = INVALID_ID;
        header.geometries[g->internalID].generation = INVALID_ID;
    }
}
