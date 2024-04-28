#pragma once

#include "defines.h"

#define KEY_MAX_CHARS 50

typedef struct hashtable {
    u64 elementStride;
    u32 elementLength;
    b8 isPointerData;
    void* memory;
} hashtable;

typedef struct entry {
    char key[KEY_MAX_CHARS];
    void* value;
} entry;

FSNAPI void hashtableCreate(u64 elementStride, u32 elementLength, void* memory, b8 isPointerData, hashtable* outHashtable);
FSNAPI void hashtableDestroy(hashtable* ht);

/** @brief Set a hashtable entry by using a name key; will overwrite; does not look for collisions or do any linear probing*/
FSNAPI b8 hashtableSet(hashtable* ht, const char* key, void* value);
FSNAPI b8 hashtableGet(hashtable* ht, const char* key, void* outValue);
FSNAPI b8 hashtableGetID(hashtable* ht, const char* key, u64* outValue);
FSNAPI void hashtableClear(hashtable* ht, const char* key);
