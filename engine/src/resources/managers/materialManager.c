#include "materialManager.h"
#include "core/fmemory.h"
#include "core/fstring.h"
#include "core/logger.h"
#include "math/fsnmath.h"
#include "platform/filesystem.h"
#include "resources/resourcesTypes.h"
#include "systems/materialSystem.h"
#include "systems/shaderSystem.h"
#include "systems/textureSystem.h"

b8 materialManagerLoad(resourceManager* self, const char* name,
                       resource* outResource) {
    char* fmtStr = "../Assets/%s.fmat";
    char fileLocation[512];
    strFmt(fileLocation, fmtStr, name);
    fileHandle f;
    if (!fsOpen(fileLocation, FILE_MODE_READ, false, &f)) {
        FERROR("Could not open file: %s", name);
        return false;
    }
    outResource->fullPath = strDup(fileLocation);

    material* resmat =
        fallocate(sizeof(material), MEMORY_TAG_MATERIAL_INSTANCE);
    resmat->autoDelete = true;
    resmat->diffuseColor = vec4One();
    strNCpy(resmat->name, name, MATERIAL_MAX_LENGTH);

    // Read the config file
    char lineBuffer[512] = "";
    char* c = &lineBuffer[0];
    u64 lineLen = 0;
    while (fsReadLine(&f, 511, &c, &lineLen)) {
        char* trimmed = strTrim(lineBuffer);
        lineLen = strLen(trimmed);
        // Skip blank or comments (#)
        if (lineLen < 1 || trimmed[0] == '#') {
            continue;
        }
        i32 equalIdx = strIdxOf(trimmed, '=');
        if (equalIdx == -1) {
            FERROR("Formating error in %s. Skipping Line.", fileLocation);
            continue;
        }

        char varName[64];
        fzeroMemory(varName, sizeof(char) * 64);
        strCut(varName, trimmed, 0, equalIdx);
        char* tvar = strTrim(varName);

        char value[446];
        fzeroMemory(value, sizeof(char) * 446);
        strCut(value, trimmed, equalIdx + 1, -1);
        char* tval = strTrim(value);

        if (strEqualI(tvar, "Name")) {
            strCpy(resmat->name, tval);
        } else if (strEqualI(tvar, "DiffuseMapName")) { // TODO: TEMP
            resmat->diffuseMap.texture =
                textureSystemTextureGetCreate(tval, true);
        } else if (strEqualI(tvar, "DiffuseColor")) {
            // Parse the colour
            if (!strToVec4(tval, &resmat->diffuseColor)) {
                FWARN("Error parsing diffuseColor in file '%s'. Using default "
                      "of white instead.",
                      fileLocation);
            }
        } else if (strEqualI(tvar, "shader")) {
            resmat->shaderID = shaderSystemGetID(strTrim(tval));
        }
    }
    fsClose(&f);
    resmat->generation = 0;
    resmat->refCnt = 1;

    outResource->data = resmat;
    outResource->dataSize = sizeof(material);
    outResource->name = name;
    return true;
}

void materialManagerUnload(resourceManager* self, resource* resource) {
    if (!self || !resource) {
        FWARN("Can't unload Material without self or resource structs.");
        return;
    }

    u32 pathLen = strLen(resource->fullPath);
    if (pathLen) {
        ffree(resource->fullPath, sizeof(char) * pathLen + 1,
              MEMORY_TAG_STRING);
    }

    if (resource->data) {
        ffree(resource->data, resource->dataSize, MEMORY_TAG_MATERIAL_INSTANCE);
        resource->managerID = INVALID_ID;
        resource->data = 0;
        resource->dataSize = 0;
    }
}

resourceManager materialManagerCreate() {
    resourceManager m;
    m.resourceType = RESOURCE_TYPE_MATERIAL;
    m.id = RESOURCE_TYPE_MATERIAL;
    m.load = materialManagerLoad;
    m.unload = materialManagerUnload;
    return m;
}
