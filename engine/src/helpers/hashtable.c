#include "hashtable.h"
#include "core/fmemory.h"
#include "core/fstring.h"
#include "core/logger.h"

#define INVALID_KEY "INVALID_KEY"

void hashtableFillEntry(hashtable* ht, entry* value);

u64 hash(const char* key, u32 elementCnt) {
    int primeCnt = 26;
    const unsigned int primes[] = {
        53,        97,        193,      389,       769,       1543,
        3079,      6151,      12289,    24593,     49157,     98317,
        196613,    393241,    786433,   1572869,   3145739,   6291469,
        12582917,  25165843,  50331653, 100663319, 201326611, 402653189,
        805306457, 1610612741};

    const unsigned char* x;

    u64 hash = 0;
    for (x = (const unsigned char*)key; *x; x++) {
        int y = (*x % 26);
        unsigned int test = primes[y];
        hash = hash * primes[y] + *x;
    }

    hash = hash % elementCnt;

    return hash;
}

void hashtableCreate(u64 elementStride, u32 elementLength, void* memory,
                     b8 isPointerData, u64* memReq, hashtable* outHashtable) {
    if (memReq != 0) {
        *memReq = (sizeof(entry) + elementStride) * elementLength;
    }
    if (memory == 0) {
        return;
    }
    outHashtable->elementStride = elementStride;
    outHashtable->elementLength = elementLength;
    outHashtable->isPointerData = isPointerData;
    outHashtable->memory = memory;
    fsetMemory(outHashtable->memory, 0, sizeof(entry) * elementLength);
    entry e;
    strCpy(e.key, INVALID_KEY);
    e.value = 0;
    hashtableFillEntry(outHashtable, &e);
}

void hashtableDestroy(hashtable* ht) {
    ffree(ht->memory, sizeof(entry) * ht->elementLength, MEMORY_TAG_ARRAY);
    ffree(ht, sizeof(hashtable), MEMORY_TAG_ARRAY);
}

b8 hashtableSet(hashtable* ht, const char* key, void* value) {
    char temp[KEY_MAX_CHARS];
    strNCpy(temp, key, KEY_MAX_CHARS);
    int h = hash(temp, ht->elementLength);
    int y = h;
    for (int i = 0; i < ht->elementLength; i++) {
        entry* e = (entry*)(ht->memory + (sizeof(entry) * y));

        if (strEqual(e->key, temp) || strEqual(e->key, INVALID_KEY)) {
            entry t;
            strNCpy(t.key, temp, KEY_MAX_CHARS);
            t.value = value;
            fcopyMemory(e, &t, sizeof(entry));
            return true;
        }
        y++;
        y = y % ht->elementLength;
    }
    FERROR("ERROR: Could not find key entry or a free slot for key: %s", key);
    return false;
}

b8 hashtableGet(hashtable* ht, const char* key, void* outValue) {
    char temp[KEY_MAX_CHARS];
    strNCpy(temp, key, KEY_MAX_CHARS);
    int h = hash(temp, ht->elementLength);
    int y = h;
    for (int i = 0; i < ht->elementLength; i++) {
        entry* e = (entry*)(ht->memory + (sizeof(entry) * y));
        if (strEqual(e->key, INVALID_KEY)) {
            return false;
        }
        if (strEqual(e->key, key)) {
            fcopyMemory(outValue, e->value, ht->elementStride);
            return true;
        }
        y++;
        y = y % ht->elementLength;
    }
    return false;
}

b8 hashtableGetID(hashtable* ht, const char* key, u64* outValue) {
    int h = hash(key, ht->elementLength);
    int y = h;
    for (int i = 0; i < ht->elementLength; i++) {
        entry* e = (entry*)(ht->memory + (sizeof(entry) * y));
        if (strEqual(e->key, INVALID_KEY)) {
            *outValue = y;
            return false;
        }
        if (strEqual(e->key, key)) {
            *outValue = y;
            return true;
        }
        y++;
        y = y % ht->elementLength;
    }
    return false;
}

void hashtableClear(hashtable* ht, const char* key) {
    int h = hash(key, ht->elementLength);
    int y = h;
    for (int i = 0; i < ht->elementLength; i++) {
        entry* e = (entry*)(ht->memory + (sizeof(entry) * y));
        if (strEqual(e->key, INVALID_KEY)) {
            return;
        }
        if (strEqual(e->key, key)) {
            entry t;
            strCpy(t.key, INVALID_KEY);
            t.value = 0;
            fcopyMemory(e, &t, sizeof(entry));
            return;
        }
        y++;
        y = y % ht->elementLength;
    }
}

// "Secret" function to make all the slots invalid when ht is created
void hashtableFillEntry(hashtable* ht, entry* value) {
    for (int i = 0; i < ht->elementLength; i++) {
        entry* e = (entry*)(ht->memory + (sizeof(entry) * i));
        fcopyMemory(e, value, sizeof(entry));
    }
}
