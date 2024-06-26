#include "renderer/rendererFront.h"
#include "core/fmemory.h"
#include "core/fstring.h"
#include "core/logger.h"
#include "defines.h"
#include "math/fsnmath.h"
#include "renderer/renderTypes.h"
#include "renderer/rendererBack.h"
// TODO: TEMP
#include "core/input.h"
#include "resources/resourceManager.h"
#include "systems/cameraSystem.h"
#include "systems/materialSystem.h"
#include "systems/shaderSystem.h"

typedef struct rendererSystem {
    rendererBackend rb;
    // Camera/World/Scene Projection
    mat4 projection;
    mat4 view;
    // UI Projection
    mat4 uiProjection;
    mat4 uiView;
    u32 frameBufferWidth;
    u32 frameBufferHeight;
    f32 nearClip;
    f32 farClip;
    u32 pbrID;
    u32 uiID;
    // TODO: TEMP
    // camera* camm;
    // camera* cam1;
    // camera* cam2;
    // b8 first;
} rendererSystem;

static rendererSystem* systemPtr;

b8 rendererInit(u64* memoryRequirement, void* memoryState,
                const char* appName) {
    *memoryRequirement = sizeof(rendererSystem);
    if (memoryState == 0) {
        return true;
    }
    systemPtr = memoryState;

    systemPtr->frameBufferWidth = 1280;
    systemPtr->frameBufferHeight = 720;
    systemPtr->nearClip = 0.1f;
    systemPtr->farClip = 1000.0f;
    systemPtr->projection = mat4Perspective(
        degToRad(45.0f),
        systemPtr->frameBufferWidth / (f32)systemPtr->frameBufferHeight,
        systemPtr->nearClip, systemPtr->farClip);
    systemPtr->uiProjection = mat4Orthographic(
        0, systemPtr->frameBufferWidth, systemPtr->frameBufferHeight, 0,
        -100.0f, 100.0f); // Intentionally flipped on the y axis
    systemPtr->uiView = mat4Inverse(mat4Identity());

    // Assign the pointer functions to the real functions
    if (!rendererCreate(RENDERER_BACKEND_API_VULKAN, &systemPtr->rb)) {
        FERROR("Renderer Create Failed");
        return false;
    }
    systemPtr->rb.frameNum = 0;
    // Init the renderer
    if (!systemPtr->rb.init(&systemPtr->rb, "First Game")) {
        FERROR("Renderer Init Failed");
        return false;
    }
    // TODO: TEMP
    // systemPtr->first = true;
    // cameraSystemAdd("Cam1");
    // cameraSystemAdd("Cam2");
    // systemPtr->camm = getCamera("main");
    // systemPtr->cam1 = getCamera("Cam1");
    // systemPtr->cam2 = getCamera("Cam2");
    // cameraMoveBackward(systemPtr->cam1, 30.0f);
    // cameraMoveBackward(systemPtr->cam2, 30.0f);
    // TODO: END TEMP


    resource resui;
    if (!resourceLoad(BUILTIN_MATERIAL_UI_NAME, RESOURCE_TYPE_SHADER, &resui)){
        FERROR("Failed loading ui material shader config file");
        return false;
    }
    shaderRS* sui = (shaderRS*)resui.data;
    if (!shaderSystemCreate(sui)){
        FERROR("Failed creating ui material");
        return false;
    }
    resourceUnload(&resui);
    systemPtr->uiID = shaderSystemGetID(BUILTIN_MATERIAL_UI_NAME);
    u32 proj = shaderSystemUniformIdx(shaderSystemGetByID(systemPtr->uiID), "diffuse_color");
    resource res;

    if (!resourceLoad(BUILTIN_MATERIAL_PBR_NAME, RESOURCE_TYPE_SHADER, &res)){
        FERROR("Failed loading pbr material shader config file");
        return false;
    }
    shaderRS* s = (shaderRS*)res.data;
    if (!shaderSystemCreate(s)){
        FERROR("Failed creating pbr material");
        return false;
    }
    resourceUnload(&res);
    systemPtr->pbrID = shaderSystemGetID(BUILTIN_MATERIAL_PBR_NAME);
    u32 proj2 = shaderSystemUniformIdx(shaderSystemGetByID(systemPtr->pbrID), "diffuse_color");
    return true;
}

void rendererShutdown() {
    systemPtr->rb.shutdown(&systemPtr->rb);
    rendererDestroy(&systemPtr->rb);
}

void rendererOnResize(u16 width, u16 height) {
    if (systemPtr) {
        systemPtr->projection =
            mat4Perspective(degToRad(45.0f), width / (f32)height,
                            systemPtr->nearClip, systemPtr->farClip);
        systemPtr->uiProjection =
            mat4Orthographic(0, (f32)width, (f32)height, 0, -100.0f,
                             100.0f); // Intentionally flipped on the y axis
        systemPtr->rb.resized(&systemPtr->rb, width, height);
    } else {
        FWARN("Renderer backend does not exist to accept resize: %i %i", width,
              height);
    }
}

b8 rendererDraw(renderHeader* renderHeader) {
    // If beginFrame fails the app might be able to recover
    // and sometimes when we redo the swapchain we'll want it to fail
    if (systemPtr->rb.beginFrame(&systemPtr->rb, renderHeader->deltaTime)) {
        if (!systemPtr->rb.beginRenderpass(&systemPtr->rb,
                                           BUILTIN_RENDERPASS_WORLD)) {
            FERROR("BeginRenderpass for BUILTIN_RENDERPASS_WORLD failed.");
            return false;
        }
        // TEMP
        // camera* cam = systemPtr->cam1;
        // if (systemPtr->first) {
        //     cam = systemPtr->cam1;
        // } else {
        //     cam = systemPtr->cam2;
        // }
        // f32 spd = 10.0f * renderHeader->deltaTime;
        // if (inputIsKeyPressed('K')) {
        //     systemPtr->first = !systemPtr->first;
        // }
        // if (inputIsKeyDown('W')) {
        //     cameraMoveUp(cam, spd);
        // }
        // if (inputIsKeyDown('S')) {
        //     cameraMoveDown(cam, spd);
        // }
        // if (inputIsKeyDown('A')) {
        //     cameraMoveLeft(cam, spd);
        // }
        // if (inputIsKeyDown('D')) {
        //     cameraMoveRight(cam, spd);
        // }
        // if (inputIsKeyDown('E')) {
        //     cameraMoveForward(cam, spd);
        // }
        // if (inputIsKeyDown('Q')) {
        //     cameraMoveBackward(cam, spd);
        // }
        // if (inputIsKeyDown('Z')) {
        //     cameraYaw(cam, spd);
        // }
        // if (inputIsKeyDown('X')) {
        //     cameraYaw(cam, -spd);
        // }
        // END TEMP
        if (!shaderSystemUseByID(systemPtr->pbrID)) {
            FERROR("Failed to use shader");
            return false;
        }

        if (!materialSystemUpdateGlobal(
                systemPtr->pbrID, &systemPtr->projection, &systemPtr->view)) {
            FERROR("Failed to update globals");
            return false;
        }

        u32 count = renderHeader->geometryCnt;
        for (u32 i = 0; i < count; i++) {
            material* m;
            if (renderHeader->geometries[i].geometry->material) {
                m = renderHeader->geometries[i].geometry->material;
            } else {
                m = materialSystemGetDefault();
            }

            if (!materialSystemUpdateInstance(m)) {
                FERROR("Failed to update instances");
                continue;
            }

            materialSystemUpdateLocal(m, &renderHeader->geometries[i].model);
            systemPtr->rb.drawGeometry(renderHeader->geometries[i]);
        }

        if (!systemPtr->rb.endRenderpass(&systemPtr->rb,
                                         BUILTIN_RENDERPASS_WORLD)) {
            FERROR("EndRenderpass for BUILTIN_RENDERPASS_WORLD failed.");
            return false;
        }

        // UI Renderpass
        if (!systemPtr->rb.beginRenderpass(&systemPtr->rb,
                                           BUILTIN_RENDERPASS_UI)) {
            FERROR("BeginRenderpass for BUILTIN_RENDERPASS_UI failed.");
            return false;
        }

        if (!shaderSystemUseByID(systemPtr->uiID)) {
            FERROR("Failed to use shader");
            return false;
        }

        if (!materialSystemUpdateGlobal(systemPtr->uiID, &systemPtr->projection,
                                        &systemPtr->view)) {
            FERROR("Failed to update globals");
            return false;
        }

        u32 uiCount = renderHeader->geometryCnt;
        for (u32 i = 0; i < uiCount; i++) {
            material* m;
            if (renderHeader->geometries[i].geometry->material) {
                m = renderHeader->geometries[i].geometry->material;
            } else {
                m = materialSystemGetDefault();
            }

            if (!materialSystemUpdateInstance(m)) {
                FERROR("Failed to update instances");
                continue;
            }

            materialSystemUpdateLocal(m, &renderHeader->geometries[i].model);
            systemPtr->rb.drawGeometry(renderHeader->geometries[i]);
        }

        if (!systemPtr->rb.endRenderpass(&systemPtr->rb,
                                         BUILTIN_RENDERPASS_UI)) {
            FERROR("EndRenderpass for BUILTIN_RENDERPASS_UI failed.");
            return false;
        }

        // If endFrame fails the app is fucked
        if (!systemPtr->rb.endFrame(&systemPtr->rb, renderHeader->deltaTime)) {
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

b8 rendererRenderpassID(const char* name, u8* outRenderpassID) {
    // HACK: Dont hard code this. Make renderpasses dynamic
    if (strEqualI("Renderpass.Builtin.World", name)) {
        *outRenderpassID = BUILTIN_RENDERPASS_WORLD;
        return true;
    } else if (strEqualI("Renderpass.Builtin.UI", name)) {
        *outRenderpassID = BUILTIN_RENDERPASS_UI;
        return true;
    }

    FERROR("Unknown renderpass name given. %s", name);
    *outRenderpassID = INVALID_ID_U8;
    return false;
}

b8 rendererCreateGeometry(geometry* geometry, u32 vertexStride, u32 vertexCnt,
                          const void* vertices, u32 indexStride, u32 indexCnt,
                          const void* indices) {
    return systemPtr->rb.createGeometry(geometry, vertexStride, vertexCnt,
                                        vertices, indexStride, indexCnt,
                                        indices);
}

void rendererDestroyGeometry(geometry* geometry) {
    systemPtr->rb.destroyGeometry(geometry);
}

b8 rendererShaderUniformSet(shader* s, shaderUniform u, const void* val) {
    return systemPtr->rb.shaderUniformSet(s, u, val);
}

b8 rendererShaderFreeInstanceStruct(shader* s, u32 instanceID) {
    return systemPtr->rb.shaderFreeInstanceStruct(s, instanceID);
}

b8 rendererShaderAllocateInstanceStruct(shader* s, u32* outInstanceID) {
    return systemPtr->rb.shaderAllocateInstanceStruct(s, outInstanceID);
}

b8 rendererShaderCreate(shader* shader, u8 renderpassID, u8 stageCnt,
                        shaderStage* stages, const char** stageFilenames) {
    return systemPtr->rb.shaderCreate(shader, renderpassID, stageCnt, stages,
                                      stageFilenames);
}

b8 rendererShaderDestroy(shader* shader) {
    return systemPtr->rb.shaderDestroy(shader);
}

b8 rendererShaderInit(shader* shader) {
    return systemPtr->rb.shaderInit(shader);
}

b8 rendererShaderUse(shader* shader) {
    return systemPtr->rb.shaderUse(shader);
}

b8 rendererShaderApplyGlobals(shader* shader) {
    return systemPtr->rb.shaderApplyGlobals(shader);
}

b8 rendererShaderApplyInstance(shader* shader) {
    return systemPtr->rb.shaderApplyInstance(shader);
}

b8 rendererShaderBindGlobals(shader* shader) {
    return systemPtr->rb.shaderBindGlobals(shader);
}

b8 rendererShaderBindInstance(shader* shader, u32 instanceID) {
    return systemPtr->rb.shaderBindInstance(shader, instanceID);
}

b8 rendererSetUniform(struct shader* s, struct shaderUniform* uniform,
                      const void* value) {
    return systemPtr->rb.shaderUniformSet(s, *uniform, value);
}
