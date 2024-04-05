#include "fmemory.h"

#include "core/dyncamicAllocator.h"
#include "core/logger.h"
#include "platform/platform.h"

// TODO: Custom string lib
#include <stdio.h>

typedef struct memoryStats {
    u64 total_allocated;
    u64 tagged_allocations[MEMORY_TAG_MAX_TAGS];
} memoryStats;

typedef struct memorySystemState {
    memoryStats stats;
    u64 allocCnt;
    memorySystemSettings settings;
    u64 allocatorMemReq;
    void* allocatorBlock;
    dynaAllocator allocator;
} memorySystemState;

static memorySystemState* systemPtr;

b8 memoryInit(memorySystemSettings settings) {
    u64 stateMemReq = sizeof(memorySystemState);
    u64 ar = 0;
    dynaAllocCreate(settings.totalSize, &ar, 0, 0);

    void* block = platformAllocate(stateMemReq + ar, false);

    systemPtr = (memorySystemState*)block;
    systemPtr->settings = settings;
    systemPtr->allocCnt = 0;
    systemPtr->allocatorMemReq = ar;

    platformZeroMemory(&systemPtr->stats, sizeof(systemPtr->stats));

    systemPtr->allocatorBlock = ((void*)block + stateMemReq);

    if (!dynaAllocCreate(settings.totalSize, &systemPtr->allocatorMemReq,
                         systemPtr->allocatorBlock, &systemPtr->allocator)) {
        FFATAL("MemoryInit Failed to allocate a dynamicAllocator.");
        return false;
    }
    FDEBUG("Memory System allocated %llu bytes", settings.totalSize);
    return true;
}

void memoryShutdown() {
    if (systemPtr) {
        dynaAllocDestroy(&systemPtr->allocator);
        platformFree(systemPtr,
                     systemPtr->allocatorMemReq + sizeof(memorySystemState));
    }
    systemPtr = 0;
}

void* fallocate(u64 size, memoryTag tag) {
    if (tag == MEMORY_TAG_UNKNOWN) {
        FWARN("fallocate called using MEMORY_TAG_UNKNOWN. Re-class this "
              "allocation.");
    }

    void* block = 0;
    if (systemPtr) {
        systemPtr->stats.total_allocated += size;
        systemPtr->stats.tagged_allocations[tag] += size;
        systemPtr->allocCnt++;

        block = dynaAlloc(&systemPtr->allocator, size);

        // As a fallback incase the dynamicAllocator fails which it should never
        // do
        if (!block) {
            FWARN("Fallocate called before memory system was inited.");
            block = platformAllocate(size, false);
        }
    }

    if (block) {
        platformZeroMemory(block, size);
        return block;
    }
    FFATAL("Fallocate failed to allocate.");
    return 0;
}

void ffree(void* block, u64 size, memoryTag tag) {
    if (tag == MEMORY_TAG_UNKNOWN) {
        FWARN(
            "ffree called using MEMORY_TAG_UNKNOWN. Re-class this allocation.");
    }

    if (systemPtr) {
        systemPtr->stats.total_allocated -= size;
        systemPtr->stats.tagged_allocations[tag] -= size;

        b8 result = dynaAllocFree(&systemPtr->allocator, size, block);

        // If result is false then that means something was allocated before the
        // memory system was inited. Try to free it from the platform.
        if (!result) {
            // TODO: Memory alignment
            platformFree(block, false);
        }
    } else {
        // TODO: Memory alignment
        platformFree(block, false);
    }
}

void* fzeroMemory(void* block, u64 size) {
    return platformZeroMemory(block, size);
}

void* fcopyMemory(void* dest, const void* source, u64 size) {
    return platformCopyMemory(dest, source, size);
}

void* fsetMemory(void* dest, i32 value, u64 size) {
    return platformSetMemory(dest, value, size);
}

void printMemoryUsage() {
    const u64 gib = 1073741824; // 1024 * 1024 * 1024
    const u64 mib = 1048576;    // 1024 * 1024
    const u64 kib = 1024;

    for (u32 i = 0; i < MEMORY_TAG_MAX_TAGS; ++i) {
        char unit[4] = "XiB";
        float amount = 1.0f;
        if (systemPtr->stats.tagged_allocations[i] >= gib) {
            unit[0] = 'G';
            amount = systemPtr->stats.tagged_allocations[i] / (float)gib;
        } else if (systemPtr->stats.tagged_allocations[i] >= mib) {
            unit[0] = 'M';
            amount = systemPtr->stats.tagged_allocations[i] / (float)mib;
        } else if (systemPtr->stats.tagged_allocations[i] >= kib) {
            unit[0] = 'K';
            amount = systemPtr->stats.tagged_allocations[i] / (float)kib;
        } else {
            unit[0] = 'B';
            unit[1] = 0;
            amount = (float)systemPtr->stats.tagged_allocations[i];
        }

        printf("  %-30s: %.2f%s\n", TAG_STRING[i], amount, unit);
    }
}
