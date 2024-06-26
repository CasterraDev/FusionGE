#include "helpers/dinoArray.h"
#include "core/fmemory.h"
#include "core/logger.h"

void* _dino_create(u64 length, u64 stride){
    //Like an html network header
    //Stores info
    u64 header = DINOARRAY_FIELD_LENGTH * sizeof(u64);
    u64 mix = (length * stride) + header;
    u64* newArr = fallocate(mix,MEMORY_TAG_DINO);
    fsetMemory(newArr, 0, mix);
    //Set header info
    newArr[DINOARRAY_MAX_SIZE] = length;
    newArr[DINOARRAY_LENGTH] = 0; //Length of current elements
    newArr[DINOARRAY_STRIDE] = stride;
    //Move the array up so the user can access their elements immediately
    return ((void*)(newArr + DINOARRAY_FIELD_LENGTH));
}

void _dino_destroy(void* array){
    u64* header = (u64*)array - DINOARRAY_FIELD_LENGTH;
    u64 headerSize = (DINOARRAY_FIELD_LENGTH * sizeof(u64));
    u64 elementSize = (header[DINOARRAY_MAX_SIZE] * header[DINOARRAY_STRIDE]);
    u64 total = headerSize + elementSize;
    ffree(header,total,MEMORY_TAG_DINO);
}

void* _dino_resize(void* array){
    u64 length = dinoLength(array);
    u64 stride = dinoStride(array);
    void* temp = _dino_create(dinoMaxSize(array) * DINO_DEFAULT_RESIZE_FACTOR,stride);
    fcopyMemory(temp,array,length * stride);
    _dino_destroy(array);
    dinoLengthSet(temp,length);
    return temp;
}

void* _dino_shrink(void* array){
    u64 length = dinoLength(array);
    u64 stride = dinoStride(array);
    void* temp = _dino_create(length + 1,stride);
    fcopyMemory(temp,array,length * stride);
    _dino_destroy(array);
    dinoLengthSet(temp,length);
    return temp;
}

u64 _dino_field_get(void* array, u64 field){
    u64* header = (u64*)array - DINOARRAY_FIELD_LENGTH;
    return (header[field]);
}

void _dino_field_set(void* array, u64 field, u64 value){
    u64* header = (u64*)array - DINOARRAY_FIELD_LENGTH;
    header[field] = value;
}

void* _dino_push(void* array, const void* valuePtr){
    u64 length = dinoLength(array);
    u64 stride = dinoStride(array);
    if (length >= DINOARRAY_MAX_SIZE){
        array = _dino_resize(array);
    }
    u64 idx = (u64)array;
    //Since length is One-based and array is Zero-based we don't have to add one for the new element
    idx += length * stride;
    fcopyMemory((void*)idx,valuePtr,stride);
    dinoLengthSet(array,length+1);
    return array;
}

void* _dino_insert_at(void* array, u64 idx, void* valuePtr){
    u64 length = dinoLength(array);
    u64 stride = dinoStride(array);
    if (idx >= length){
        FERROR("DINO ERROR: Index was more than array length")
        return array;
    }
    if (length >= DINOARRAY_MAX_SIZE){
        array = _dino_resize(array);
    }
    u64 memIdx = (u64)array;

    //If idx isn't at the end move elements after it down one
    //Element after the index to move the afterbit to
    u64 elementAfter = memIdx + ((idx+1) * stride);
    u64 afterbit = memIdx + (idx * stride);
    if (idx != length - 1){
        fcopyMemory((void*)elementAfter, (void*)afterbit, stride * (length - idx));
    }
    //Actually copy the idx value into the array
    fcopyMemory((void*)memIdx + (idx * stride),valuePtr,stride);
    dinoLengthSet(array,length+1);
    return array;
}

void _dino_pop(void* array, void* dest){
    u64 length = dinoLength(array);
    u64 stride = dinoStride(array);
    u64 idx = (u64)array;
    idx += (length - 1) * stride;
    fcopyMemory(dest,(void*)idx,stride);
    dinoLengthSet(array,length-1);
}

void* _dino_pop_at(void* array, u64 idx, void* dest){
    u64 length = dinoLength(array);
    if (idx >= length){
        FERROR("DINO ERROR: Index was more than array length")
        return array;
    }
    u64 stride = dinoStride(array);
    u64 memIdx = (u64)array;
    u64 eleIdx = idx * stride;
    //Copy the element to the dest
    fcopyMemory(dest,(void*)(memIdx + eleIdx),stride);

    //If idx isn't at the end move elements after it down one
    //Element after the index to move the afterbit to
    u64 elementAfter = memIdx + ((idx+1) * stride);
    u64 afterbit = memIdx + (idx * stride);
    if (idx != length - 1){
        fcopyMemory((void*)afterbit, (void*)elementAfter, stride * (length - idx));
    }
    dinoLengthSet(array,length-1);
    return array;
}
