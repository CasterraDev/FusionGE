#pragma once

#include "defines.h"
//Make sure MEMORY_TAG_MAX_TAGS is ALWAYS at the end. It's used as a sort of null pointer for loops
#define FOREACH_TAG(TAG)                \
    TAG(MEMORY_TAG_UNKNOWN)             \
    TAG(MEMORY_TAG_ARRAY)               \
    TAG(MEMORY_TAG_DINO)                \
    TAG(MEMORY_TAG_DICT)                \
    TAG(MEMORY_TAG_RING_QUEUE)          \
    TAG(MEMORY_TAG_BST)                 \
    TAG(MEMORY_TAG_STRING)              \
    TAG(MEMORY_TAG_APPLICATION)         \
    TAG(MEMORY_TAG_JOB)                 \
    TAG(MEMORY_TAG_TEXTURE)             \
    TAG(MEMORY_TAG_MATERIAL_INSTANCE)   \
    TAG(MEMORY_TAG_ALLOCATORS)          \
    TAG(MEMORY_TAG_FILE_DATA)           \
    TAG(MEMORY_TAG_RENDERER)            \
    TAG(MEMORY_TAG_GAME)                \
    TAG(MEMORY_TAG_TRANSFORM)           \
    TAG(MEMORY_TAG_ENTITY)              \
    TAG(MEMORY_TAG_ENTITY_NODE)         \
    TAG(MEMORY_TAG_SCENE)               \
    TAG(MEMORY_TAG_MAX_TAGS)            \

#define GENERATE_ENUM(ENUM) ENUM,
#define GENERATE_STRING(STRING) #STRING,

typedef enum memoryTag {
    // For temporary use. Should be assigned one of the below or have a new tag created.
    FOREACH_TAG(GENERATE_ENUM)
} memoryTag;

static const char *TAG_STRING[] = {
    FOREACH_TAG(GENERATE_STRING)
};

FSNAPI void memoryInit(u64* memoryRequirement, void* state);
FSNAPI void memoryShutdown();

FSNAPI void* fallocate(u64 size, memoryTag tag);

FSNAPI void ffree(void* block, u64 size, memoryTag tag);

FSNAPI void* fzeroMemory(void* block, u64 size);

FSNAPI void* fcopyMemory(void* dest, const void* source, u64 size);

FSNAPI void* fsetMemory(void* dest, i32 value, u64 size);

//WARN: Memory Leak
FSNAPI char* getMemoryUsageStr();

FSNAPI void printMemoryUsage();