#include "geometrySystem.h"

#include "core/fmemory.h"
#include "core/fstring.h"
#include "core/logger.h"
#include "materialSystem.h"
#include "renderer/rendererFront.h"

typedef struct geometryReference {
    u64 referenceCount;
    geometry geometry;
    b8 autoRelease;
} geometryReference;

typedef struct geometrySystemState {
    geometrySystemConfig config;

    geometry defaultGeometry;
    geometry defaultGeometry2D;

    // Array of registered meshes.
    geometryReference* registeredGeometries;
} geometrySystemState;

static geometrySystemState* systemPtr = 0;

b8 createDefaultGeometries(geometrySystemState* state);
b8 createGeometry(geometrySystemState* state, geometryConfig config,
                  geometry* g);
void destroyGeometry(geometrySystemState* state, geometry* g);

b8 geometrySystemInit(u64* memoryRequirement, void* state,
                      geometrySystemConfig config) {
    if (config.maxGeometryCnt == 0) {
        FFATAL("geometrySystemInitialize - config.maxGeometryCnt must be > 0.");
        return false;
    }

    // Block of memory will contain state structure, then block for array, then
    // block for hashtable.
    u64 structRequirement = sizeof(geometrySystemState);
    u64 arrayRequirement = sizeof(geometryReference) * config.maxGeometryCnt;
    *memoryRequirement = structRequirement + arrayRequirement;

    if (!state) {
        return true;
    }

    systemPtr = state;
    systemPtr->config = config;

    // The array block is after the state. Already allocated, so just set the
    // pointer.
    void* arrayBlock = state + structRequirement;
    systemPtr->registeredGeometries = arrayBlock;

    // Invalidate all geometries in the array.
    u32 count = systemPtr->config.maxGeometryCnt;
    for (u32 i = 0; i < count; ++i) {
        systemPtr->registeredGeometries[i].geometry.id = INVALID_ID;
        systemPtr->registeredGeometries[i].geometry.internalID = INVALID_ID;
        systemPtr->registeredGeometries[i].geometry.generation = INVALID_ID;
    }

    if (!createDefaultGeometries(systemPtr)) {
        FFATAL(
            "Failed to create default geometry. Application cannot continue.");
        return false;
    }

    return true;
}

void geometrySystemShutdown(void* state) {
    // NOTE: nothing to do here.
}

geometry* geometrySystemAcquireById(u32 id) {
    if (id != INVALID_ID &&
        systemPtr->registeredGeometries[id].geometry.id != INVALID_ID) {
        systemPtr->registeredGeometries[id].referenceCount++;
        return &systemPtr->registeredGeometries[id].geometry;
    }

    // NOTE: Should return default geometry instead?
    FERROR("geometrySystemAcquireById cannot load invalid geometry id. "
           "Returning nullptr.");
    return 0;
}

geometry* geometrySystemAcquireFromConfig(geometryConfig config,
                                          b8 autoRelease) {
    geometry* g = 0;
    for (u32 i = 0; i < systemPtr->config.maxGeometryCnt; ++i) {
        if (systemPtr->registeredGeometries[i].geometry.id == INVALID_ID) {
            // Found empty slot.
            systemPtr->registeredGeometries[i].autoRelease = autoRelease;
            systemPtr->registeredGeometries[i].referenceCount = 1;
            g = &systemPtr->registeredGeometries[i].geometry;
            g->id = i;
            break;
        }
    }

    if (!g) {
        FERROR("Unable to obtain free slot for geometry. Adjust configuration "
               "to allow more space. Returning nullptr.");
        return 0;
    }

    if (!createGeometry(systemPtr, config, g)) {
        FERROR("Failed to create geometry. Returning nullptr.");
        return 0;
    }

    return g;
}

void geometrySystemRelease(geometry* geometry) {
    if (geometry && geometry->id != INVALID_ID) {
        geometryReference* ref = &systemPtr->registeredGeometries[geometry->id];

        // Take a copy of the id;
        u32 id = geometry->id;
        if (ref->geometry.id == geometry->id) {
            if (ref->referenceCount > 0) {
                ref->referenceCount--;
            }

            // Also blanks out the geometry id.
            if (ref->referenceCount < 1 && ref->autoRelease) {
                destroyGeometry(systemPtr, &ref->geometry);
                ref->referenceCount = 0;
                ref->autoRelease = false;
            }
        } else {
            FFATAL("Geometry id mismatch. Check registration logic, as this "
                   "should never occur.");
        }
        return;
    }

    FWARN("geometrySystemAcquireById cannot release invalid geometry id. "
          "Nothing was done.");
}

geometry* geometrySystemGetDefault() {
    if (systemPtr) {
        return &systemPtr->defaultGeometry;
    }

    FFATAL("geometrySystemGetDefault called before system was initialized. "
           "Returning nullptr.");
    return 0;
}

geometry* geometrySystemGetDefault2D() {
    if (systemPtr) {
        return &systemPtr->defaultGeometry2D;
    }

    FFATAL("geometrySystemGetDefault2D called before system was initialized. "
           "Returning nullptr.");
    return 0;
}

b8 createGeometry(geometrySystemState* state, geometryConfig config,
                  geometry* g) {
    // Send the geometry off to the renderer to be uploaded to the GPU.
    if (!rendererCreateGeometry(g, config.vertexStride, config.vertexCnt,
                                config.vertices, config.indexStride,
                                config.indexCnt, config.indices)) {
        // Invalidate the entry.
        state->registeredGeometries[g->id].referenceCount = 0;
        state->registeredGeometries[g->id].autoRelease = false;
        g->id = INVALID_ID;
        g->generation = INVALID_ID;
        g->internalID = INVALID_ID;

        return false;
    }

    // Acquire the material
    if (strLen(config.materialName) > 0) {
        g->material = materialSystemMaterialGet(config.materialName);
        if (!g->material) {
            g->material = materialSystemGetDefault();
        }
    }

    return true;
}

void destroyGeometry(geometrySystemState* state, geometry* g) {
    rendererDestroyGeometry(g);
    g->internalID = INVALID_ID;
    g->generation = INVALID_ID;
    g->id = INVALID_ID;

    strEmpty(g->name);

    // Release the material.
    if (g->material && strLen(g->material->name) > 0) {
        materialSystemMaterialRelease(g->material->name);
        g->material = 0;
    }
}

b8 createDefaultGeometries(geometrySystemState* state) {
    vertex3D verts[4];
    fzeroMemory(verts, sizeof(vertex3D) * 4);

    const f32 f = 10.0f;

    verts[0].position.x = -0.5 * f; // 0    3
    verts[0].position.y = -0.5 * f; //
    verts[0].texcoord.x = 0.0f;     //
    verts[0].texcoord.y = 0.0f;     // 2    1

    verts[1].position.y = 0.5 * f;
    verts[1].position.x = 0.5 * f;
    verts[1].texcoord.x = 1.0f;
    verts[1].texcoord.y = 1.0f;

    verts[2].position.x = -0.5 * f;
    verts[2].position.y = 0.5 * f;
    verts[2].texcoord.x = 0.0f;
    verts[2].texcoord.y = 1.0f;

    verts[3].position.x = 0.5 * f;
    verts[3].position.y = -0.5 * f;
    verts[3].texcoord.x = 1.0f;
    verts[3].texcoord.y = 0.0f;

    u32 indices[6] = {0, 1, 2, 0, 3, 1};

    // Send the geometry off to the renderer to be uploaded to the GPU.
    if (!rendererCreateGeometry(&state->defaultGeometry, sizeof(vertex3D), 4,
                                verts, sizeof(u32), 6, indices)) {
        FFATAL(
            "Failed to create default geometry. Application cannot continue.");
        return false;
    }

    // Acquire the default material.
    state->defaultGeometry.material = materialSystemGetDefault();

    vertex2D verts2D[4];
    fzeroMemory(verts2D, sizeof(vertex2D) * 4);

    verts2D[0].position.x = -0.5 * f; // 0    3
    verts2D[0].position.y = -0.5 * f; //
    verts2D[0].texcoord.x = 0.0f;     //
    verts2D[0].texcoord.y = 0.0f;     // 2    1

    verts2D[1].position.y = 0.5 * f;
    verts2D[1].position.x = 0.5 * f;
    verts2D[1].texcoord.x = 1.0f;
    verts2D[1].texcoord.y = 1.0f;

    verts2D[2].position.x = -0.5 * f;
    verts2D[2].position.y = 0.5 * f;
    verts2D[2].texcoord.x = 0.0f;
    verts2D[2].texcoord.y = 1.0f;

    verts2D[3].position.x = 0.5 * f;
    verts2D[3].position.y = -0.5 * f;
    verts2D[3].texcoord.x = 1.0f;
    verts2D[3].texcoord.y = 0.0f;

    // Indices (Counter-Clockwise)
    u32 indices2D[6] = {2, 1, 0, 3, 0, 1};

    // Send the geometry off to the renderer to be uploaded to the GPU.
    if (!rendererCreateGeometry(&state->defaultGeometry2D, sizeof(vertex2D), 4,
                                verts2D, sizeof(u32), 6, indices2D)) {
        FFATAL("Failed to create default geometry2D. Application cannot "
               "continue.");
        return false;
    }

    // Acquire the default material.
    state->defaultGeometry2D.material = materialSystemGetDefault();

    return true;
}

geometryConfig geometrySystemGeneratePlaneConfig(f32 width, f32 height,
                                                 u32 xSegmentCount,
                                                 u32 ySegmentCount, f32 tileX,
                                                 f32 tileY, const char* name,
                                                 const char* materialName) {
    if (width == 0) {
        FWARN("Width must be nonzero. Defaulting to one.");
        width = 1.0f;
    }
    if (height == 0) {
        FWARN("Height must be nonzero. Defaulting to one.");
        height = 1.0f;
    }
    if (xSegmentCount < 1) {
        FWARN("xSegmentCount must be a positive number. Defaulting to one.");
        xSegmentCount = 1;
    }
    if (ySegmentCount < 1) {
        FWARN("ySegmentCount must be a positive number. Defaulting to one.");
        ySegmentCount = 1;
    }

    if (tileX == 0) {
        FWARN("tileX must be nonzero. Defaulting to one.");
        tileX = 1.0f;
    }
    if (tileY == 0) {
        FWARN("tileY must be nonzero. Defaulting to one.");
        tileY = 1.0f;
    }

    geometryConfig config;
    config.vertexStride = sizeof(vertex3D);
    config.vertexCnt = xSegmentCount * ySegmentCount * 4; // 4 verts per segment
    config.vertices =
        fallocate(sizeof(vertex3D) * config.vertexCnt, MEMORY_TAG_ARRAY);
    config.indexStride = sizeof(u32);
    config.indexCnt =
        xSegmentCount * ySegmentCount * 6; // 6 indices per segment
    config.indices = fallocate(sizeof(u32) * config.indexCnt, MEMORY_TAG_ARRAY);

    // TODO: This generates extra vertices, but we can always deduplicate them
    // later.
    f32 segWidth = width / xSegmentCount;
    f32 segHeight = height / ySegmentCount;
    f32 halfWidth = width * 0.5f;
    f32 halfHeight = height * 0.5f;
    for (u32 y = 0; y < ySegmentCount; ++y) {
        for (u32 x = 0; x < xSegmentCount; ++x) {
            // Generate vertices
            f32 minX = (x * segWidth) - halfWidth;
            f32 minY = (y * segHeight) - halfHeight;
            f32 maxX = minX + segWidth;
            f32 maxY = minY + segHeight;
            f32 minUvx = (x / (f32)xSegmentCount) * tileX;
            f32 minUvy = (y / (f32)ySegmentCount) * tileY;
            f32 maxUvx = ((x + 1) / (f32)xSegmentCount) * tileX;
            f32 maxUvy = ((y + 1) / (f32)ySegmentCount) * tileY;

            u32 vOffset = ((y * xSegmentCount) + x) * 4;
            vertex3D* v0 = &((vertex3D*)config.vertices)[vOffset + 0];
            vertex3D* v1 = &((vertex3D*)config.vertices)[vOffset + 1];
            vertex3D* v2 = &((vertex3D*)config.vertices)[vOffset + 2];
            vertex3D* v3 = &((vertex3D*)config.vertices)[vOffset + 3];

            v0->position.x = minX;
            v0->position.y = minY;
            v0->texcoord.x = minUvx;
            v0->texcoord.y = minUvy;

            v1->position.x = maxX;
            v1->position.y = maxY;
            v1->texcoord.x = maxUvx;
            v1->texcoord.y = maxUvy;

            v2->position.x = minX;
            v2->position.y = maxY;
            v2->texcoord.x = minUvx;
            v2->texcoord.y = maxUvy;

            v3->position.x = maxX;
            v3->position.y = minY;
            v3->texcoord.x = maxUvx;
            v3->texcoord.y = minUvy;

            // Generate indices
            u32 iOffset = ((y * xSegmentCount) + x) * 6;
            ((u32*)config.indices)[iOffset + 0] = vOffset + 0;
            ((u32*)config.indices)[iOffset + 1] = vOffset + 1;
            ((u32*)config.indices)[iOffset + 2] = vOffset + 2;
            ((u32*)config.indices)[iOffset + 3] = vOffset + 0;
            ((u32*)config.indices)[iOffset + 4] = vOffset + 3;
            ((u32*)config.indices)[iOffset + 5] = vOffset + 1;
        }
    }

    if (name && strLen(name) > 0) {
        strNCpy(config.name, name, GEOMETRY_NAME_MAX_LENGTH);
    } else {
        strNCpy(config.name, DEFAULT_GEOMETRY_NAME, GEOMETRY_NAME_MAX_LENGTH);
    }

    if (materialName && strLen(materialName) > 0) {
        strNCpy(config.materialName, materialName, GEOMETRY_NAME_MAX_LENGTH);
    } else {
        strNCpy(config.materialName, DEFAULT_MATERIAL_NAME,
                GEOMETRY_NAME_MAX_LENGTH);
    }

    return config;
}
