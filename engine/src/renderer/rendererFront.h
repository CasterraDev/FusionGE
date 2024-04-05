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

b8 rendererCreateGeometry(geometry* geometry, u32 vertexStride, u32 vertexCnt, const void* vertices, u32 indexStride, u32 indexCnt, const void* indices);

void rendererDestroyGeometry(geometry* geometry);