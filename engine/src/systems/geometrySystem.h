#pragma once

#include "defines.h"
#include "resources/resourcesTypes.h"

typedef struct geometrySystemConfig {
    // Max number of geometries that can be loaded at once.
    // NOTE: Should be significantly greater than the number of static meshes because
    // the there can and will be more than one of these per mesh.
    // Take other systems into acCnt as well
    u32 maxGeometryCnt;
} geometrySystemConfig;

typedef struct geometryConfig {
    u32 vertexStride;
    u32 vertexCnt;
    void* vertices;
    u32 indexStride;
    u32 indexCnt;
    void* indices;
    char name[GEOMETRY_NAME_MAX_LENGTH];
    char materialName[GEOMETRY_NAME_MAX_LENGTH];
} geometryConfig;

b8 geometrySystemInit(u64* memoryRequirement, void* state, geometrySystemConfig config);
void geometrySystemShutdown(void* state);

/**
 * @brief Acquires an existing geometry by id.
 * 
 * @param id The geometry identifier to acquire by.
 * @return A pointer to the acquired geometry or nullptr if failed.
 */
geometry* geometrySystemAcquireById(u32 id);

/**
 * @brief Registers and acquires a new geometry using the given config.
 * 
 * @param config The geometry configuration.
 * @param autoRelease Indicates if the acquired geometry should be unloaded when its reference Cnt reaches 0.
 * @return A pointer to the acquired geometry or nullptr if failed. 
 */
geometry* geometrySystemAcquireFromConfig(geometryConfig config, b8 autoRelease);

/**
 * @brief Releases a reference to the provided geometry.
 * 
 * @param geometry The geometry to be released.
 */
void geometrySystemRelease(geometry* geometry);

/**
 * @brief Obtains a pointer to the default geometry.
 * 
 * @return A pointer to the default geometry. 
 */
geometry* geometrySystemGetDefault();

/**
 * @brief Obtains a pointer to the default geometry2D.
 * 
 * @return A pointer to the default geometry2D. 
 */
geometry* geometrySystemGetDefault2D();

/**
 * @brief Generates configuration for plane geometries given the provided parameters.
 * NOTE: vertex and index arrays are dynamically allocated and should be freed upon object disposal.
 * Thus, this should not be considered production code.
 * 
 * @param width The overall width of the plane. Must be non-zero.
 * @param height The overall height of the plane. Must be non-zero.
 * @param xSegmentCnt The number of segments along the x-axis in the plane. Must be non-zero.
 * @param ySegmentCnt The number of segments along the y-axis in the plane. Must be non-zero.
 * @param tileX The number of times the texture should tile across the plane on the x-axis. Must be non-zero.
 * @param tileY The number of times the texture should tile across the plane on the y-axis. Must be non-zero.
 * @param name The name of the generated geometry.
 * @param materialName The name of the material to be used.
 * @return A geometry configuration which can then be fed into geometrySystemAcquireFromConfig().
 */
geometryConfig geometrySystemGeneratePlaneConfig(f32 width, f32 height, u32 xSegmentCnt, u32 ySegmentCnt, f32 tileX, f32 tileY, const char* name, const char* materialName);
