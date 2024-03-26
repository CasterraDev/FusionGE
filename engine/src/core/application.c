#include "application.h"
#include "gameTypes.h"

#include "logger.h"

#include "platform/platform.h"
#include "core/fmemory.h"
#include "core/linearAllocator.h"
#include "core/event.h"
#include "core/input.h"
#include "core/clock.h"
#include "core/fstring.h"
#include "renderer/rendererFront.h"

#include "systems/cameraSystem.h"
#include "systems/textureSystem.h"
#include "systems/materialSystem.h"
#include "systems/geometrySystem.h"
#include "resources/resourceManager.h"

//TODO: TEMP
#include "math/fsnmath.h"

typedef struct appState {
    game* gameInstance;
    clock clock;
    b8 isRunning;
    b8 isSuspended;
    i16 width;
    i16 height;
    f64 lastTime;

    linearAllocator subSystemsAllocator;

    u64 loggerSystemMemoryRequirement;
    void* loggerSystemPtr;
    u64 eventSystemMemoryRequirement;
    void* eventSystemPtr;
    u64 memorySystemMemoryRequirement;
    void* memorySystemPtr;
    u64 inputSystemMemoryRequirement;
    void* inputSystemPtr;
    u64 platformSystemMemoryRequirement;
    void* platformSystemPtr;
    u64 resourceManagerMemoryRequirement;
    void* resourceManagerPtr;
    u64 rendererSystemMemoryRequirement;
    void* rendererSystemPtr;

    linearAllocator systemsAllocator;

    u64 cameraSystemMemoryRequirement;
    void* cameraSystemPtr;
    u64 textureSystemMemoryRequirement;
    void* textureSystemPtr;
    u64 materialSystemMemoryRequirement;
    void* materialSystemPtr;
    u64 geometrySystemMemoryRequirement;
    void* geometrySystemPtr;

    // TODO: temp
    geometry* testGeometry;
    // TODO: end temp
} appState;

static appState* appstate;

b8 applicationOnEvent(u16 code, void* sender, void* listenerInst, eventContext context);
b8 applicationOnKey(u16 code, void* sender, void* listenerInst, eventContext context);
b8 applicationKeyHeld(u16 code, void* sender, void* listenerInst, eventContext context);

b8 applicationOnResized(u16 code, void* sender, void* listenerInst, eventContext context);

// TODO: temp
b8 eventDebug(u16 code, void* sender, void* listener_inst, eventContext data) {
    const char* names[4] = {
        "paving.png",
        "cobblestone.png",
        "paving2.png",
        "Bricks085.png"};
    static i8 choice = 2;

    // Save off the old name.
    const char* old_name = appstate->testGeometry->material->diffuseMap.texture->name;

    choice++;
    choice %= 4;
    FINFO("GETTING NEW TEXTURE MAP");
    // Acquire the new texture.
    if (appstate->testGeometry){
        appstate->testGeometry->material->diffuseMap.texture = textureSystemTextureGetCreate(names[choice], true);
        if (!appstate->testGeometry->material->diffuseMap.texture->data) {
            FWARN("event_on_debug_event no texture! using default");
            appstate->testGeometry->material->diffuseMap.texture = textureSystemGetDefault();
        }
        FTRACE("New texture: %s", appstate->testGeometry->material->diffuseMap.texture->name);

        // Release the old texture.
        textureSystemTextureRelease(old_name);
    }

    return true;
}
// TODO: end temp

b8 appCreate(game* gameInst) {
    if (appstate) {
        FERROR("appCreate called more than once.");
        return false;
    }
    appstate = fallocate(sizeof(appState), MEMORY_TAG_APPLICATION);
    appstate->gameInstance = gameInst;
    appstate->isRunning = true;
    appstate->isSuspended = false;

    //Initialize subsystems.

    //Get memory requirements for all subsystems
    u32 subsystemsSize = 0;
    loggerInit(&appstate->loggerSystemMemoryRequirement, 0);
    subsystemsSize += appstate->loggerSystemMemoryRequirement;
    eventInit(&appstate->eventSystemMemoryRequirement, 0);
    subsystemsSize += appstate->eventSystemMemoryRequirement;

    memoryInit(&appstate->memorySystemMemoryRequirement, 0);
    subsystemsSize += appstate->memorySystemMemoryRequirement;

    inputInit(&appstate->inputSystemMemoryRequirement, 0);
    subsystemsSize += appstate->inputSystemMemoryRequirement;

    platformStartup(
        &appstate->platformSystemMemoryRequirement,
        0,
        gameInst->appConfig.name,
        gameInst->appConfig.startPosX,
        gameInst->appConfig.startPosY,
        gameInst->appConfig.startWidth,
        gameInst->appConfig.startHeight);
    subsystemsSize += appstate->platformSystemMemoryRequirement;

    resourceManagerSettings rsSettings;
    rsSettings.maxManagers = 32;
    rsSettings.rootAssetPath = "../Assets/";
    resourceManagerInit(&appstate->resourceManagerMemoryRequirement, 0, rsSettings);
    subsystemsSize += appstate->resourceManagerMemoryRequirement;
    
    rendererInit(&appstate->rendererSystemMemoryRequirement, 0, appstate->gameInstance->appConfig.name);
    subsystemsSize += appstate->rendererSystemMemoryRequirement;

    //Create the allocater for the needed required memory for all subsystems
    linearAllocCreate(subsystemsSize, &appstate->subSystemsAllocator);

    //"Allocate" the memory and give each subsystem it's ptr
    appstate->loggerSystemPtr = linearAllocAllocate(&appstate->subSystemsAllocator, appstate->loggerSystemMemoryRequirement);
    appstate->eventSystemPtr = linearAllocAllocate(&appstate->subSystemsAllocator, appstate->eventSystemMemoryRequirement);
    appstate->memorySystemPtr = linearAllocAllocate(&appstate->subSystemsAllocator, appstate->memorySystemMemoryRequirement);
    appstate->inputSystemPtr = linearAllocAllocate(&appstate->subSystemsAllocator, appstate->inputSystemMemoryRequirement);
    appstate->platformSystemPtr = linearAllocAllocate(&appstate->subSystemsAllocator, appstate->platformSystemMemoryRequirement);
    appstate->resourceManagerPtr = linearAllocAllocate(&appstate->subSystemsAllocator, appstate->resourceManagerMemoryRequirement);
    appstate->rendererSystemPtr = linearAllocAllocate(&appstate->subSystemsAllocator, appstate->rendererSystemMemoryRequirement);

    //Actually init all subsystems
    loggerInit(&appstate->loggerSystemMemoryRequirement, appstate->loggerSystemPtr);
    eventInit(&appstate->eventSystemMemoryRequirement, appstate->eventSystemPtr);
    memoryInit(&appstate->memorySystemMemoryRequirement, appstate->memorySystemPtr);
    inputInit(&appstate->inputSystemMemoryRequirement, appstate->inputSystemPtr);

    //Register the app events before starting the platform
    eventRegister(EVENT_CODE_APPLICATION_QUIT, 0, applicationOnEvent);
    eventRegister(EVENT_CODE_KEY_DOWN, 0, applicationKeyHeld);
    eventRegister(EVENT_CODE_KEY_PRESSED, 0, applicationOnKey);
    eventRegister(EVENT_CODE_KEY_RELEASED, 0, applicationOnKey);
    eventRegister(EVENT_CODE_RESIZED, 0, applicationOnResized);
    // TODO: temp
    eventRegister(EVENT_CODE_DEBUG0, 0, eventDebug);
    // TODO: end temp

    platformStartup(
        &appstate->platformSystemMemoryRequirement,
        appstate->platformSystemPtr,
        gameInst->appConfig.name,
        gameInst->appConfig.startPosX,
        gameInst->appConfig.startPosY,
        gameInst->appConfig.startWidth,
        gameInst->appConfig.startHeight);
    
    resourceManagerInit(&appstate->resourceManagerMemoryRequirement, appstate->resourceManagerPtr, rsSettings);

    //TODO: Place rendererInit here

    //Init all systems
    u64 systemsMemRequirement = 0;
    cameraSystemInit(&appstate->cameraSystemMemoryRequirement, 0, 24);
    systemsMemRequirement += appstate->cameraSystemMemoryRequirement;
    textureSystemSettings textureSettings;
    textureSettings.maxTextureCnt = 1024;
    textureSystemInit(&appstate->textureSystemMemoryRequirement, 0, textureSettings);
    systemsMemRequirement += appstate->textureSystemMemoryRequirement;
    materialSystemSettings matSettings;
    matSettings.maxMaterialCnt = 1024;
    materialSystemInit(&appstate->materialSystemMemoryRequirement, 0, matSettings);
    systemsMemRequirement += appstate->materialSystemMemoryRequirement;
    geometrySystemConfig geoConfig;
    geoConfig.maxGeometryCnt = 4096;
    geometrySystemInitialize(&appstate->geometrySystemMemoryRequirement, 0, geoConfig);
    systemsMemRequirement += appstate->geometrySystemMemoryRequirement;


    linearAllocCreate(systemsMemRequirement, &appstate->systemsAllocator);
    
    appstate->cameraSystemPtr = linearAllocAllocate(&appstate->systemsAllocator, appstate->cameraSystemMemoryRequirement);
    cameraSystemInit(&appstate->cameraSystemMemoryRequirement, appstate->cameraSystemPtr, 24);
    //TODO: TEMPorary renderer is placed here cuz some temp code in it needs the camera system inited. Place under second platformStartup call
    rendererInit(&appstate->rendererSystemMemoryRequirement, appstate->rendererSystemPtr, appstate->gameInstance->appConfig.name);
    appstate->textureSystemPtr = linearAllocAllocate(&appstate->systemsAllocator, appstate->textureSystemMemoryRequirement);
    textureSystemInit(&appstate->textureSystemMemoryRequirement, appstate->textureSystemPtr, textureSettings);
    appstate->materialSystemPtr = linearAllocAllocate(&appstate->systemsAllocator, appstate->materialSystemMemoryRequirement);
    materialSystemInit(&appstate->materialSystemMemoryRequirement, appstate->materialSystemPtr, matSettings);

    // Geometry system.
    appstate->geometrySystemPtr = linearAllocAllocate(&appstate->systemsAllocator, appstate->geometrySystemMemoryRequirement);
    if (!geometrySystemInitialize(&appstate->geometrySystemMemoryRequirement, appstate->geometrySystemPtr, geoConfig)) {
        FFATAL("Failed to initialize geometry system. Application cannot continue.");
        return false;
    }

    // TODO: temp 

    // Load up a plane configuration, and load geometry from it.
    geometryConfig g_config = geometrySystemGeneratePlaneConfig(10.0f, 5.0f, 5, 5, 5.0f, 2.0f, "test geometry", "material");
    appstate->testGeometry = geometrySystemAcquireFromConfig(g_config, true);

    // Clean up the allocations for the geometry config.
    ffree(g_config.vertices, sizeof(vertex3D) * g_config.vertexCnt, MEMORY_TAG_ARRAY);
    ffree(g_config.indices, sizeof(u32) * g_config.indexCnt, MEMORY_TAG_ARRAY);

    // Load up default geometry.
    //app_state->test_geometry = geometry_system_get_default();
    // TODO: end temp 


    // Init the game.
    if (!appstate->gameInstance->initialize(appstate->gameInstance)) {
        FFATAL("Game failed to init.");
        return false;
    }

    appstate->gameInstance->onResize(appstate->gameInstance, appstate->width, appstate->height);

    return true;
}

b8 appRun() {
    clockStart(&appstate->clock);
    clockUpdate(&appstate->clock);
    appstate->lastTime = appstate->clock.elapsed;
    f64 runningTime = 0;
    u8 frameCount = 0;
    f64 targetFrameSec = 1.0f / 60;

    printMemoryUsage();
    while (appstate->isRunning) {
        if(!platformPumpMessages()) {
            appstate->isRunning = false;
        }

        if(!appstate->isSuspended) {
            // Update clock and get delta time.
            clockUpdate(&appstate->clock);
            f64 curTime = appstate->clock.elapsed;
            f64 delta = (curTime - appstate->lastTime);
            f64 frameStartTime = platformGetAbsoluteTime();

            if (!appstate->gameInstance->update(appstate->gameInstance, (f32)delta)) {
                FFATAL("Game update failed, shutting down.");
                appstate->isRunning = false;
                break;
            }

            // Call the game's render routine.
            if (!appstate->gameInstance->render(appstate->gameInstance, (f32)delta)) {
                FFATAL("Game render failed, shutting down.");
                appstate->isRunning = false;
                break;
            }
            
            renderHeader header;
            header.deltaTime = delta;
            // TODO: temp
            geometryRenderData testGeoHeader;
            testGeoHeader.geometry = appstate->testGeometry;
            testGeoHeader.model = mat4Identity();

            header.geometryCnt = 1;
            header.geometries = &testGeoHeader;

            header.uiGeometryCnt = 0;
            header.uiGeometries = 0;
            // TODO: end temp
            rendererDraw(&header);

            // Figure out how long the frame took and, if below
            f64 frameEndTime = platformGetAbsoluteTime();
            f64 frameElapsedTime = frameEndTime - frameStartTime;
            runningTime += frameElapsedTime;
            f64 remainingSec = targetFrameSec - frameElapsedTime;

            if (remainingSec > 0) {
                u64 remaining_ms = (remainingSec * 1000);

                // If there is time left, give it back to the OS.
                b8 limitFrames = false;
                if (remaining_ms > 0 && limitFrames) {
                    platformSleep(remaining_ms - 1);
                }

                frameCount++;
            }

            
            inputUpdate(0);

            appstate->lastTime = curTime;
        }
    }

    FINFO("Got out of appstate->isrunning loop");
    appstate->isRunning = false;

    eventUnregister(EVENT_CODE_APPLICATION_QUIT, 0, applicationOnEvent);
    eventUnregister(EVENT_CODE_KEY_DOWN, 0, applicationKeyHeld);
    eventUnregister(EVENT_CODE_KEY_PRESSED, 0, applicationOnKey);
    eventUnregister(EVENT_CODE_KEY_RELEASED, 0, applicationOnKey);
    eventUnregister(EVENT_CODE_RESIZED, 0, applicationOnResized);
    // TODO: temp
    eventUnregister(EVENT_CODE_DEBUG0, 0, eventDebug);
    // TODO: end temp

    inputShutdown(appstate->inputSystemPtr);

    geometrySystemShutdown(appstate->geometrySystemPtr);
    materialSystemShutdown(appstate->materialSystemPtr);

    textureSystemShutdown(appstate->textureSystemPtr);
    cameraSystemShutdown(appstate->cameraSystemPtr);

    rendererShutdown();
    resourceManagerShutdown(appstate->resourceManagerPtr);
    platformShutdown();
    memoryShutdown();
    eventShutdown();
    loggerShutdown();

    return true;
}

b8 applicationKeyHeld(u16 code, void* sender, void* listenerInst, eventContext context){
    u16 keyCode = context.data.u16[0];
    FDEBUG("'%c' | '%d' being held",keyCode,keyCode);
    return true;
}

void appGetFramebufferSize(u32* width, u32* height){
    *width = appstate->width;
    *height = appstate->height;
}

b8 applicationOnEvent(u16 code, void* sender, void* listenerInst, eventContext context) {
    switch (code) {
        case EVENT_CODE_APPLICATION_QUIT: {
            FINFO("EVENT_CODE_APPLICATION_QUIT recieved, shutting down.\n");
            appstate->isRunning = false;
            return true;
        }
    }

    return false;
}

b8 applicationOnKey(u16 code, void* sender, void* listenerInst, eventContext context) {
    if (code == EVENT_CODE_KEY_PRESSED) {
        u16 keyCode = context.data.u16[0];
        if (keyCode == KEY_ESCAPE) {
            // NOTE: Technically firing an event to itself, but there may be other listeners.
            eventContext data = {};
            eventFire(EVENT_CODE_APPLICATION_QUIT, 0, data);

            // Block anything else from processing this.
            return true;
        } else if (keyCode == KEY_A) {
            // Example on checking for a key
            FDEBUG("Explicit - A key pressed!");
        } else {
            FDEBUG("'%c' | '%d' key pressed in window.", keyCode, keyCode);
        }
    } else if (code == EVENT_CODE_KEY_RELEASED) {
        u16 keyCode = context.data.u16[0];
        if (keyCode == KEY_B) {
            // Example on checking for a key
            FDEBUG("Explicit - B key released!");
        } else {
            FDEBUG("'%c' | '%d' key released in window.", keyCode, keyCode);
        }
    }
    return false;
}

b8 applicationOnResized(u16 code, void* sender, void* listenerInst, eventContext context){
    if (code == EVENT_CODE_RESIZED){
        u16 width = context.data.u16[0];
        u16 height = context.data.u16[1];

        //Check if different. If so, then trigger the resize event.
        if (width != appstate->width || height != appstate->height){
            appstate->width = width;
            appstate->height = height;

            FDEBUG("Window resized: %i, %i", width, height);

            //Handle minimization
            if (width == 0 && height == 0){
                FINFO("Window minimized, suspending application");
                appstate->isSuspended = true;
                return true;
            }else{
                if (appstate->isSuspended){
                    FINFO("Window restored, resuming application");
                    appstate->isSuspended = false;
                }
                appstate->gameInstance->onResize(appstate->gameInstance, width, height);
                rendererOnResize(width, height);
            }
        }
    }

    //Event purposely not handled to allow other listeners to get to it.
    return false;
}
