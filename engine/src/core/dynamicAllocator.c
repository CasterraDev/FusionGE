#include "core/fmemory.h"
#include "core/logger.h"
#include "dyncamicAllocator.h"
#include "helpers/freelist.h"

b8 dynaAllocCreate(u64 totalSize, u64* memoryRequirement, void* memory,
                   dynaAllocator* outAllocator) {
    // Get the memoryRequirement for the freelist
    u64 freelistReq = 0;
    freelistCreate(totalSize, &freelistReq, 0, 0);

    *memoryRequirement = freelistReq + sizeof(dynaAllocator) + totalSize;

    if (!memory) {
        return true;
    }

    outAllocator->totalSize = totalSize;
    outAllocator->freelistBlock = (void*)(memory + sizeof(dynaAllocator));
    outAllocator->memoryBlock = (void*)(outAllocator->freelistBlock + freelistReq);

    freelistCreate(totalSize, &freelistReq, outAllocator->freelistBlock,
                   &outAllocator->list);

    fzeroMemory(outAllocator->memoryBlock, totalSize);
    return true;
}

b8 dynaAllocDestroy(dynaAllocator* allocator) {
    if (allocator) {
        freelistDestroy(&allocator->list);
        fzeroMemory(allocator->memoryBlock, allocator->totalSize);
        allocator->totalSize = 0;
        allocator->memoryBlock = 0;
        return true;
    }
    FWARN("DynaAllocDestroy failed to free/zero the allocator.");
    return false;
}

void* dynaAlloc(dynaAllocator* alloc, u64 size) {
    if (alloc && size > 0) {
        u64 offset = 0;
        if (freelistAllocateBlock(&alloc->list, size, &offset)) {
            void* block = (void*)(alloc->memoryBlock + offset);
            return block;
        } else {
            // TODO: Report some stuff about the dynaAllocator
            // for easier debugging since the User is gonna be able to use this
            FERROR("Failed to allocate with DynaAlloc.");
            return 0;
        }
    }

    FERROR("DynaAlloc needs an allocator and a size above 0.");

    return false;
}

b8 dynaAllocFree(dynaAllocator* alloc, u64 size, void* memory) {
    u64 offset = memory - alloc->memoryBlock;
    if (!freelistFreeBlock(&alloc->list, size, offset)) {
        FERROR("DynaAllocFree failed to free block.");
        return false;
    }
    return true;
}

u64 dynaAllocFreeSpace(dynaAllocator* alloc) {
    return freelistFreeSpace(&alloc->list);
}
