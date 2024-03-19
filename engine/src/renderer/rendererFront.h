#pragma once

#include "renderTypes.h"

struct platformState;

b8 rendererInit(u64* memoryRequirement, void* memoryState, const char* appName);
void rendererShutdown();

void rendererOnResize(u16 width, u16 height);

b8 rendererDraw(renderHeader* header);

void rendererCreateTexture(const u8* pixels, struct texture* texture);
void rendererDestroyTexture(struct texture* texture);

b8 rendererCreateMaterial(struct material* m);
void rendererDestroyMaterial(struct material* m);

b8 rendererCreateGeometry(geometry* geometry, u32 vertexCnt, const vertex3D* vertices, u32 indexCnt, const u32* indices);

void rendererDestroyGeometry(geometry* geometry);