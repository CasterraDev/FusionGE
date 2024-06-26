#pragma once

#include "defines.h"

enum{
    DINOARRAY_MAX_SIZE,
    DINOARRAY_LENGTH,
    DINOARRAY_STRIDE,
    DINOARRAY_FIELD_LENGTH
};

//"Private Functions" will wrap with define functions

FSNAPI void* _dino_create(u64 length, u64 stride);
FSNAPI void _dino_destroy(void* array);
FSNAPI void* _dino_resize(void* array);
FSNAPI void* _dino_shrink(void* array);

FSNAPI u64 _dino_field_get(void* array, u64 field);
FSNAPI void _dino_field_set(void* array, u64 field, u64 value);

FSNAPI void* _dino_push(void* array, const void* valuePtr);
FSNAPI void _dino_pop(void* array, void* dest);

FSNAPI void* _dino_pop_at(void* array, u64 idx, void* dest);
FSNAPI void* _dino_insert_at(void* array, u64 idx, void* valuePtr);

#define DINO_DEFAULT_SIZE 1
#define DINO_DEFAULT_RESIZE_FACTOR 2

//Define function wrappers
#define dinoCreate(type) \
    _dino_create(DINO_DEFAULT_SIZE,sizeof(type));

#define dinoCreateSize(size) \
    _dino_create(DINO_DEFAULT_SIZE, size);

#define dinoCreateReserve(length,type) \
    _dino_create(length,sizeof(type));

#define dinoDestroy(array) _dino_destroy(array);

#define dinoShrink(array) _dino_shrink(array);

#define dinoPush(array, value)           \
    {                                       \
        typeof(value) t = value;         \
        array = _dino_push(array, &t); \
    }

#define dinoPop(array, valuePtr) \
    _dino_pop(array, valuePtr)

#define dinoInsertAt(array, index, value)           \
    {                                                   \
        typeof(value) temp = value;                     \
        array = _dino_insert_at(array, index, &temp); \
    }

#define dinoPopAt(array, index, value_ptr) \
    _dino_pop_at(array, index, value_ptr)

//QOL Functions Defined

#define dinoClear(array) \
    _dino_field_set(array, DINOARRAY_LENGTH, 0)

#define dinoLengthSet(array, value) \
    _dino_field_set(array, DINOARRAY_LENGTH, value)

//QOL Variables Defined

#define dinoMaxSize(array) \
    _dino_field_get(array, DINOARRAY_MAX_SIZE)

#define dinoLength(array) \
    _dino_field_get(array, DINOARRAY_LENGTH)

#define dinoStride(array) \
    _dino_field_get(array, DINOARRAY_STRIDE)
