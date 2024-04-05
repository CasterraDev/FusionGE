#include "renderer/rendererFront.h"
#include "renderer/rendererBack.h"
#include "math/fsnmath.h"
#include "core/logger.h"
//TODO: TEMP
#include "systems/cameraSystem.h"
#include "core/input.h"

typedef struct rendererSystem{
    rendererBackend rb;
    // Camera/World/Scene Projection
    mat4 projection;
    // UI Projection
    mat4 uiProjection;
    mat4 uiView;
    u32 frameBufferWidth;
    u32 frameBufferHeight;
    f32 nearClip;
    f32 farClip;
    //TODO: TEMP
    camera* camm;
    camera* cam1;
    camera* cam2;
    b8 first;
} rendererSystem;

static rendererSystem* systemPtr;

b8 rendererInit(u64* memoryRequirement, void* memoryState, const char* appName){
    *memoryRequirement = sizeof(rendererSystem);
    if (memoryState == 0){
        return true;
    }
    systemPtr = memoryState;

    systemPtr->frameBufferWidth = 1280;
    systemPtr->frameBufferHeight = 720;
    systemPtr->nearClip = 0.1f;
    systemPtr->farClip = 1000.0f;
    systemPtr->projection = mat4Perspective(degToRad(45.0f), systemPtr->frameBufferWidth / (f32)systemPtr->frameBufferHeight, systemPtr->nearClip, systemPtr->farClip);
    systemPtr->uiProjection = mat4Orthographic(0, systemPtr->frameBufferWidth, systemPtr->frameBufferHeight, 0, -100.0f, 100.0f); // Intentionally flipped on the y axis
    systemPtr->uiView = mat4Inverse(mat4Identity());

    //Assign the pointer functions to the real functions
    if(!rendererCreate(RENDERER_BACKEND_API_VULKAN, &systemPtr->rb)){
        FERROR("Renderer Create Failed");
        return false;
    }
    systemPtr->rb.frameNum = 0;
    //Init the renderer
    if (!systemPtr->rb.init(&systemPtr->rb, "First Game")){
        FERROR("Renderer Init Failed");
        return false;
    }
    //TODO: TEMP
    systemPtr->first = true;
    cameraSystemAdd("Cam1");
    cameraSystemAdd("Cam2");
    systemPtr->camm = getCamera("main");
    systemPtr->cam1 = getCamera("Cam1");
    systemPtr->cam2 = getCamera("Cam2");
    cameraMoveBackward(systemPtr->cam1, 30.0f);
    cameraMoveBackward(systemPtr->cam2, 30.0f);
    //TODO: END TEMP
    return true;
}

void rendererShutdown(){
    systemPtr->rb.shutdown(&systemPtr->rb);
    rendererDestroy(&systemPtr->rb);
}

void rendererOnResize(u16 width, u16 height){
    if (systemPtr){
        systemPtr->projection = mat4Perspective(degToRad(45.0f), width / (f32)height, systemPtr->nearClip, systemPtr->farClip);
        systemPtr->uiProjection = mat4Orthographic(0, (f32)width, (f32)height, 0, -100.0f, 100.0f); // Intentionally flipped on the y axis
	    systemPtr->rb.resized(&systemPtr->rb, width, height);
    }else{
        FWARN("Renderer backend does not exist to accept resize: %i %i", width, height);
    }
}

b8 rendererDraw(renderHeader* renderHeader){
    //If beginFrame fails the app might be able to recover
    //and sometimes when we redo the swapchain we'll want it to fail
	if (systemPtr->rb.beginFrame(&systemPtr->rb, renderHeader->deltaTime)){
        if (!systemPtr->rb.beginRenderpass(&systemPtr->rb, BUILTIN_RENDERPASS_WORLD)){
            FERROR("BeginRenderpass for BUILTIN_RENDERPASS_WORLD failed.");
            return false;
        }
        // TEMP
        camera* cam = systemPtr->cam1;
        if (systemPtr->first){
            cam = systemPtr->cam1;
        }else{
            cam = systemPtr->cam2;
        }
        f32 spd = 10.0f * renderHeader->deltaTime;
        if (inputIsKeyPressed('K')){
            systemPtr->first = !systemPtr->first;
        }
        if (inputIsKeyDown('W')){
            cameraMoveUp(cam, spd);
        }
        if (inputIsKeyDown('S')){
            cameraMoveDown(cam, spd);
        }
        if (inputIsKeyDown('A')){
            cameraMoveLeft(cam, spd);
        }
        if (inputIsKeyDown('D')){
            cameraMoveRight(cam, spd);
        }
        if (inputIsKeyDown('E')){
            cameraMoveForward(cam, spd);
        }
        if (inputIsKeyDown('Q')){
            cameraMoveBackward(cam, spd);
        }
        if (inputIsKeyDown('Z')){
            cameraYaw(cam, spd);
        }
        if (inputIsKeyDown('X')){
            cameraYaw(cam, -spd);
        }
        // END TEMP
        systemPtr->rb.updateGlobalState(systemPtr->projection, cameraViewGet(cam), vec3Zero(), vec4One(), 0);
        
        u32 count = renderHeader->geometryCnt;
        for (u32 i = 0; i < count; i++){
            systemPtr->rb.drawGeometry(renderHeader->geometries[i]);
        }

        if (!systemPtr->rb.endRenderpass(&systemPtr->rb, BUILTIN_RENDERPASS_WORLD)){
            FERROR("EndRenderpass for BUILTIN_RENDERPASS_WORLD failed.");
            return false;
        }

        // UI Renderpass
        if (!systemPtr->rb.beginRenderpass(&systemPtr->rb, BUILTIN_RENDERPASS_UI)){
            FERROR("BeginRenderpass for BUILTIN_RENDERPASS_UI failed.");
            return false;
        }

        systemPtr->rb.updateGlobalUIState(systemPtr->uiProjection, systemPtr->uiView, 0);

        u32 uiCount = renderHeader->uiGeometryCnt;
        for (u32 i = 0; i < uiCount; i++){
            systemPtr->rb.drawGeometry(renderHeader->uiGeometries[i]);
        }

        if (!systemPtr->rb.endRenderpass(&systemPtr->rb, BUILTIN_RENDERPASS_UI)){
            FERROR("EndRenderpass for BUILTIN_RENDERPASS_UI failed.");
            return false;
        }

        //If endFrame fails the app is fucked
        if (!systemPtr->rb.endFrame(&systemPtr->rb, renderHeader->deltaTime)){
            FERROR("Renderer Draw Frame Failed");
            return false;
        }
        systemPtr->rb.frameNum++;
    }
    return true;
}

void rendererCreateTexture(const u8* pixels, struct texture* texture) {
    systemPtr->rb.createTexture(pixels, texture);
}

void rendererDestroyTexture(struct texture* texture) {
    systemPtr->rb.destroyTexture(texture);
}

b8 rendererCreateMaterial(struct material* m){
    return systemPtr->rb.createMaterial(m);
}

void rendererDestroyMaterial(struct material* m){
    systemPtr->rb.destroyMaterial(m);
}

b8 rendererCreateGeometry(geometry* geometry, u32 vertexStride, u32 vertexCnt, const void* vertices, u32 indexStride, u32 indexCnt, const void* indices) {
    return systemPtr->rb.createGeometry(geometry, vertexStride, vertexCnt, vertices, indexStride, indexCnt, indices);
}

void rendererDestroyGeometry(geometry* geometry) {
    systemPtr->rb.destroyGeometry(geometry);
}
