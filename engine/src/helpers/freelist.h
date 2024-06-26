#pragma once

#include "defines.h"

/**
 * @brief A data structure to be used alongside an allocator for dynamic memory
 * allocation. Tracks free ranges of memory.
 */
typedef struct freelist {
    /** @brief The internal state of the freelist. */
    void* memory;
} freelist;

/**
 * @brief Creates a new freelist or obtains the memory requirement for one. Call
 * twice; once passing 0 to memory to obtain memory requirement, and a second
 * time passing an allocated block to memory.
 *
 * @param totalSize The total size in bytes that the free list should track.
 * @param memoryRequirement A pointer to hold memory requirement for the free
 * list itself.
 * @param memory 0, or a pre-allocated block of memory for the free list to use.
 * @param outList A pointer to hold the created free list.
 */
FSNAPI void freelistCreate(u64 totalSize, u64* memoryReq, void* memory,
                           freelist* outList);

/**
 * @brief Destroys the provided list.
 *
 * @param list The list to be destroyed.
 */
FSNAPI void freelistDestroy(freelist* list);

/**
 * @brief Attempts to find a free block of memory of the given size. Doesn't
 * actual allocate anything
 *
 * @param list A pointer to the list to search.
 * @param size The size to allocate.
 * @param outOffset A pointer to hold the offset to the allocated memory.
 * @return b8 True if a block of memory was found and allocated; otherwise
 * false.
 */
FSNAPI b8 freelistAllocateBlock(freelist* list, u64 size, u64* outOffset);

/**
 * @brief Attempts to free a block of memory at the given offset, and of the
 * given size. Can fail if invalid data is passed.
 *
 * @param list A pointer to the list to free from.
 * @param size The size to be freed.
 * @param offset The offset to free at.
 * @return b8 True if successful; otherwise false. False should be treated as an
 * error.
 */
FSNAPI b8 freelistFreeBlock(freelist* list, u64 size, u64 offset);

/**
 * @brief Resizes a new freelist or obtains the new memory requirement for one. Call
 * twice; once passing 0 to newMemory to obtain new memory requirement, and a second
 * time passing an allocated block to memory.
 * NOTE: Must free oldMemory yourself.
 * NOTE: Should be called twice to query memory requirements
 *
 * @param list The freelist to resize
 * @param memoryReq A pointer to hold memory requirement for the free
 * list itself.
 * @param size The size in bytes that the free list should be resized to.
 * @param newMemory 0, or a pre-allocated block of memory for the free list to use.
 * @param oldMemory 0, or an allocated block of memory that should be FREED externally after this function.
 */
FSNAPI b8 freelistResize(freelist* list, u64* memoryReq, u64 size, void* newMemory, void** oldMemory);

/**
 * @brief Clears the free list.
 *
 * @param list The list to be cleared.
 */
FSNAPI void freelistClear(freelist* list);

/**
 * @brief Returns the amount of free space in this list. NOTE: Since this has
 * to iterate the entire internal list, this can be an expensive operation.
 * Use sparingly.
 *
 * @param list A pointer to the list to obtain from.
 * @return The amount of free space in bytes.
 */
FSNAPI u64 freelistFreeSpace(freelist* list);
