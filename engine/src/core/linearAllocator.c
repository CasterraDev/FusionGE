#include "linearAllocator.h"
#include "fmemory.h"
#include "logger.h"

void linearAllocCreate(u64 size, linearAllocator* outLinearAlloc){
    if (outLinearAlloc){
        outLinearAlloc->memory = fallocate(size, MEMORY_TAG_ALLOCATORS);
        outLinearAlloc->size = size;
        outLinearAlloc->alloced = 0;
        return;
    }

    FERROR("linearAllocCreate was not given an outLinearAlloc")
}

void* linearAllocAllocate(linearAllocator* linearAlloc, u64 size){
    if (linearAlloc && linearAlloc->memory){
        if (linearAlloc->alloced + size > linearAlloc->size){
            u64 remainingSpace = linearAlloc->size - linearAlloc->alloced;
            FERROR("Linear allocator out of memory. Remaining space: %lluB", remainingSpace);
            return 0;
        }
        void* p = ((u8*)linearAlloc->memory) + linearAlloc->alloced;
        linearAlloc->alloced += size;
        return p;
    }
    FERROR("Linear allocator not allocated or inited");
    return 0;
}

void linearAllocReset(linearAllocator* linearAlloc){
    if (linearAlloc && linearAlloc->memory){
        fzeroMemory(linearAlloc->memory,linearAlloc->size);
        linearAlloc->alloced = 0;
    }
}

FSNAPI void linearAllocDestroy(linearAllocator* linearAlloc){
    if (linearAlloc && linearAlloc->memory){
        ffree(linearAlloc->memory,linearAlloc->size,MEMORY_TAG_ALLOCATORS);
        linearAlloc->memory = 0;
        linearAlloc->size = 0;
        linearAlloc->alloced = 0;
    }
}
