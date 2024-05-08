#include "rendererBack.h"

#include "core/fmemory.h"
#include "vulkan/backend.h"

b8 rendererCreate(rendererBackendAPI api, rendererBackend* rb){
    if (api == RENDERER_BACKEND_API_VULKAN){
        rb->init = vulkanInit;
        rb->shutdown = vulkanShutdown;
        rb->resized = vulkanResized;
        rb->drawGeometry = vulkanDrawGeometry;
        rb->beginFrame = vulkanBeginFrame;
        rb->endFrame = vulkanEndFrame;
        rb->beginRenderpass = vulkanBeginRenderpass;
        rb->endRenderpass = vulkanEndRenderpass;
        rb->createTexture = vulkanCreateTexture;
        rb->destroyTexture = vulkanDestroyTexture;
        rb->createGeometry = vulkanCreateGeometry;
        rb->destroyGeometry = vulkanDestroyGeometry;


		rb->shaderCreate = vulkanShaderCreate;
		rb->shaderDestroy = vulkanShaderDestroy;
		rb->shaderInit = vulkanShaderInit;
		rb->shaderUse = vulkanShaderUse;
		rb->shaderApplyGlobals = vulkanShaderApplyGlobals;
		rb->shaderApplyInstance = vulkanShaderApplyInstance;
		rb->shaderBindGlobals = vulkanShaderBindGlobals;
		rb->shaderBindInstance = vulkanShaderBindInstance;
		rb->shaderUniformSet = vulkanShaderUniformSet;
		rb->shaderFreeInstanceStruct = vulkanShaderFreeInstanceStruct;
		rb->shaderAllocateInstanceStruct = vulkanShaderAllocateInstanceStruct;
        return true;
    }
    return false;
}

b8 rendererDestroy(rendererBackend* rb){
    fzeroMemory(rb, sizeof(rendererBackend));
    return true;
}
