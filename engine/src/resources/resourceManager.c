#include "resourceManager.h"
#include "core/logger.h"

//Managers
#include "managers/imageManager.h"
#include "managers/binaryManager.h"
#include "managers/materialManager.h"
#include "resources/managers/shaderManager.h"

typedef struct resourceManagerState{
    resourceManagerSettings settings;
    resourceManager* loadedManagers;
} resourceManagerState;

static resourceManagerState* systemPtr = 0;

b8 resourceManagerInit(u64* memoryRequirement, void* state, resourceManagerSettings settings){
    if (settings.maxManagers <= 0){
        FERROR("Resource manager cannot be less or equal to zero.")
        return false;
    }

    *memoryRequirement = sizeof(resourceManagerState) + sizeof(resourceManager) * settings.maxManagers;

    if (!state){
        return true;
    }

    systemPtr = state;
    systemPtr->settings = settings;

    systemPtr->loadedManagers = state + sizeof(resourceManagerState);

    for(u32 i = 0; i < settings.maxManagers; i++){
        systemPtr->loadedManagers[i].id = INVALID_ID;
    }

    //Load all standard managers
    resourceManagerLoadManager(imageManagerCreate());
    resourceManagerLoadManager(binaryManagerCreate());
    resourceManagerLoadManager(materialManagerCreate());
    resourceManagerLoadManager(shaderManagerCreate());

    FINFO("Current resource manager root asset path is %s", settings.rootAssetPath);
    return true;
}

void resourceManagerShutdown(void* state){
    if (systemPtr){
        systemPtr = 0;
    }
}

b8 resourceManagerLoadManager(resourceManager manager){
    if (!systemPtr){
        FERROR("Resource manager used before being inited.");
        return false;
    }

    if (manager.resourceType == RESOURCE_TYPE_CUSTOM){
        //TODO: Implement this
        FERROR("Custom resource managers not yet implemented");
        return false;
    }

    resourceManager* m = &systemPtr->loadedManagers[manager.resourceType];

    systemPtr->loadedManagers[manager.resourceType] = manager;
    systemPtr->loadedManagers[manager.resourceType].resourceType = manager.resourceType;
    systemPtr->loadedManagers[manager.resourceType].id = manager.resourceType;
    return true;
}

b8 resourceLoad(const char* name, ResourceType type, resource* outResource){
    if (!systemPtr){
        FERROR("Resource manager used before being inited.");
        return false;
    }

    if (type == RESOURCE_TYPE_CUSTOM){
        //TODO: Implement this
        FERROR("Custom resource managers not yet implemented");
        return false;
    }

    resourceManager* m = &systemPtr->loadedManagers[type];

    if (m->id != INVALID_ID && m->load && outResource && name){
        if (!m->load(m, name, outResource)){
            return false;
        }
        outResource->managerID = type;
    }
    return true;
}

b8 resourceUnload(resource* resource){
    if (!systemPtr){
        FERROR("Resource manager used before being inited.");
        return false;
    }

    resourceManager* m = &systemPtr->loadedManagers[resource->managerID];

    if (resource && m->id != INVALID_ID && m->unload){
        m->unload(m, resource);
    }
    return true;
}

char* resourceManagerRootAssetPath(){
    if (systemPtr){
        return systemPtr->settings.rootAssetPath;
    }

    return "";
}

void resourceManagerChangeRootAssetPath(char* newRootAssetPath){
    if (systemPtr){
        systemPtr->settings.rootAssetPath = newRootAssetPath;
    }
}
