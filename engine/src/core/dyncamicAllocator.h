#pragma once

#include "defines.h"
#include "helpers/freelist.h"

typedef struct dynaAllocator {
    u64 totalSize;
    freelist list;
    void* freelistBlock;
    void* memoryBlock;
} dynaAllocator;

b8 dynaAllocCreate(u64 totalSize, u64* memoryRequirement, void* memory,
                   dynaAllocator* outAllocator);

b8 dynaAllocDestroy(dynaAllocator* allocator);

void* dynaAlloc(dynaAllocator* alloc, u64 size);

b8 dynaAllocFree(dynaAllocator* alloc, u64 size, void* memory);

u64 dynaAllocFreeSpace(dynaAllocator* alloc);
