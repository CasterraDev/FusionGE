#pragma once

#include "defines.h"

typedef struct linearAllocator{
    u64 size;
    u64 alloced;
    void* memory;
}linearAllocator;

FSNAPI void linearAllocCreate(u64 size, linearAllocator* outLinearAlloc);
FSNAPI void* linearAllocAllocate(linearAllocator* linearAlloc, u64 size);
FSNAPI void linearAllocReset(linearAllocator* linearAlloc);
FSNAPI void linearAllocDestroy(linearAllocator* linearAlloc);