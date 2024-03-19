#include "rendererBack.h"

#include "vulkan/backend.h"

b8 rendererCreate(renderer_backend_API api, rendererBackend* rb){
    if (api == RENDERER_BACKEND_API_VULKAN){
        rb->init = vulkanInit;
        rb->shutdown = vulkanShutdown;
        rb->resized = vulkanResized;
        rb->updateGlobalState = vulkanUpdateGlobalState;
        rb->drawGeometry = vulkanDrawGeometry;
        rb->beginFrame = vulkanBeginFrame;
        rb->endFrame = vulkanEndFrame;
        rb->createTexture = vulkanCreateTexture;
        rb->destroyTexture = vulkanDestroyTexture;
        rb->createMaterial = vulkanCreateMaterial;
        rb->destroyMaterial = vulkanDestroyMaterial;
        rb->createGeometry = vulkanCreateGeometry;
        rb->destroyGeometry = vulkanDestroyGeometry;

        return true;
    }
    return false;
}

b8 rendererDestroy(rendererBackend* rb){
    rb->init = 0;
    rb->shutdown = 0;
    rb->resized = 0;
    rb->updateGlobalState = 0;
    rb->drawGeometry = 0;
    rb->beginFrame = 0;
    rb->endFrame = 0;
    rb->createTexture = 0;
    rb->destroyTexture = 0;
    rb->createMaterial = 0;
    rb->destroyMaterial = 0;
    rb->createGeometry = 0;
    rb->destroyGeometry = 0;
    return true;
}