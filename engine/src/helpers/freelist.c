#include "freelist.h"

#include "core/fmemory.h"
#include "core/logger.h"

typedef struct freelistNode {
    u64 offset;
    u64 size;
    struct freelistNode* next;
} freelistNode;

typedef struct internalState {
    u64 totalSize;
    u64 maxEntries;
    freelistNode* head;
    freelistNode* nodes;
} internalState;

freelistNode* getNode(freelist* list);
void invalidateNode(freelist* list, freelistNode* node);

void freelistCreate(u64 totalSize, u64* memoryRequirement, void* memory,
                    freelist* outList) {
    u64 maxEntries = (totalSize / (sizeof(void*) * sizeof(freelistNode)));

    *memoryRequirement = sizeof(internalState) + (sizeof(freelistNode) * maxEntries);

    if (!memory) {
        return;
    }

    outList->memory = memory;

    // The block's layout is head* first, then array of available nodes.
    fzeroMemory(outList->memory, *memoryRequirement);
    internalState* state = outList->memory;
    state->nodes = (void*)(outList->memory + sizeof(internalState));
    state->maxEntries = maxEntries;
    state->totalSize = totalSize;

    state->head = &state->nodes[0];
    state->head->offset = 0;
    state->head->size = totalSize;
    state->head->next = 0;

    // Invalidate the offset and size for all but the first node. The invalid
    // value will be checked for when seeking a new node from the list.
    for (u64 i = 1; i < state->maxEntries; ++i) {
        state->nodes[i].offset = INVALID_ID;
        state->nodes[i].size = INVALID_ID;
    }
}

void freelistDestroy(freelist* list) {
    if (list && list->memory) {
        // Just zero out the memory before giving it back.
        // Since the freelist doesn't allocate any actual memory
        internalState* state = list->memory;
        fzeroMemory(list->memory, sizeof(internalState) +
                                      sizeof(freelistNode) * state->maxEntries);
        list->memory = 0;
    }
}

b8 freelistAllocateBlock(freelist* list, u64 size, u64* outOffset) {
    if (!list || !outOffset || !list->memory) {
        return false;
    }
    internalState* state = list->memory;
    freelistNode* node = state->head;
    freelistNode* previous = 0;
    while (node) {
        if (node->size == size) {
            // Exact match. Just return the node.
            *outOffset = node->offset;
            freelistNode* nodeToReturn = 0;
            if (previous) {
                previous->next = node->next;
                nodeToReturn = node;
            } else {
                // This node is the head of the list. Reassign the head
                // and return the previous head node.
                nodeToReturn = state->head;
                state->head = node->next;
            }
            invalidateNode(list, nodeToReturn);
            return true;
        } else if (node->size > size) {
            // Node is larger. Deduct the memory from it and move the offset
            // by that amount.
            *outOffset = node->offset;
            node->size -= size;
            node->offset += size;
            return true;
        }

        previous = node;
        node = node->next;
    }

    u64 freeSpace = freelistFreeSpace(list);
    FWARN("freelistFindBlock, no block large enough found (requested: %lluB, "
          "available: %lluB).",
          size, freeSpace);
    return false;
}

b8 freelistFreeBlock(freelist* list, u64 size, u64 offset) {
    if (!list || !list->memory || !size) {
        return false;
    }
    internalState* state = list->memory;
    freelistNode* node = state->head;
    freelistNode* previous = 0;
    if (!node) {
        // Check for the case where the entire thing is allocated.
        // In this case a new node is needed at the head.
        freelistNode* newNode = getNode(list);
        newNode->offset = offset;
        newNode->size = size;
        newNode->next = 0;
        state->head = newNode;
        return true;
    } else {
        while (node) {
            if (node->offset == offset) {
                // Can just be appended to this node.
                node->size += size;

                // Check if this then connects the range between this and the
                // next node, and if so, combine them and return the second
                // node..
                if (node->next &&
                    node->next->offset == node->offset + node->size) {
                    node->size += node->next->size;
                    freelistNode* next = node->next;
                    node->next = node->next->next;
                    invalidateNode(list, next);
                }
                return true;
            } else if (node->offset > offset) {
                // Iterated beyond the space to be freed. Need a new node.
                freelistNode* newNode = getNode(list);
                newNode->offset = offset;
                newNode->size = size;

                // If there is a previous node, the new node should be inserted
                // between this and it.
                if (previous) {
                    previous->next = newNode;
                    newNode->next = node;
                } else {
                    // Otherwise, the new node becomes the head.
                    newNode->next = node;
                    state->head = newNode;
                }

                // Double-check next node to see if it can be joined.
                if (newNode->next &&
                    newNode->offset + newNode->size == newNode->next->offset) {
                    newNode->size += newNode->next->size;
                    freelistNode* deletedNode = newNode->next;
                    newNode->next = deletedNode->next;
                    invalidateNode(list, deletedNode);
                }

                // Double-check previous node to see if the newNode can be
                // joined to it.
                if (previous &&
                    previous->offset + previous->size == newNode->offset) {
                    previous->size += newNode->size;
                    freelistNode* deletedNode = newNode;
                    previous->next = deletedNode->next;
                    invalidateNode(list, deletedNode);
                }

                return true;
            }

            previous = node;
            node = node->next;
        }
    }

    FWARN("Unable to find block to be freed. Corruption possible? Or the dev "
          "who called this function is stupid.");
    return false;
}

b8 freelistResize(freelist* list, u64* memoryReq, u64 size, void* newMemory, void** oldMemory){
    u64 maxEntries = (size / sizeof(void*)); // NOTE: Max amount of entries that the
                                     // freelist could have

    // Enough space for state and plus array for all nodes.
    *memoryReq = sizeof(internalState) + (sizeof(freelistNode) * maxEntries);

    if (!newMemory) {
        return true;
    }
    
    *oldMemory = list->memory;
    internalState* oldState = (internalState*)list->memory;
    u64 sizeDiff = size - oldState->totalSize;
    // Setup the new memory
    list->memory = newMemory;

    // The block's layout is head* first, then array of available nodes.
    fzeroMemory(list->memory, *memoryReq);

    // Setup the new state.
    internalState* state = (internalState*)list->memory;
    state->nodes = (void*)(list->memory + sizeof(internalState));
    state->maxEntries = maxEntries;
    state->totalSize = size;

    // Invalidate the offset and size for all but the first node. The invalid
    // value will be checked for when seeking a new node from the list.
    for (u64 i = 1; i < state->maxEntries; ++i) {
        state->nodes[i].offset = INVALID_ID;
        state->nodes[i].size = INVALID_ID;
    }

    state->head = &state->nodes[0];

    // Copy over the nodes.
    freelistNode* newListNode = state->head;
    freelistNode* oldNode = oldState->head;
    if (!oldNode) {
        // If there is no head, then the entire list is allocated. In this case,
        // the head should be set to the difference of the space now available, and
        // at the end of the list.
        state->head->offset = oldState->totalSize;
        state->head->size = sizeDiff;
        state->head->next = 0;
    } else {
        // Iterate the old nodes.
        while (oldNode) {
            // Get a new node, copy the offset/size, and set next to it.
            freelistNode* newNode = getNode(list);
            newNode->offset = oldNode->offset;
            newNode->size = oldNode->size;
            newNode->next = 0;
            newListNode->next = newNode;
            // Move to the next entry.
            newListNode = newListNode->next;

            if (oldNode->next) {
                // If there is another node, move on.
                oldNode = oldNode->next;
            } else {
                // Reached the end of the list.
                // Check if it extends to the end of the block. If so,
                // just append to the size. Otherwise, create a new node and
                // attach to it.
                if (oldNode->offset + oldNode->size == oldState->totalSize) {
                    newNode->size += sizeDiff;
                } else {
                    freelistNode* newNodeEnd = getNode(list);
                    newNodeEnd->offset = oldState->totalSize;
                    newNodeEnd->size = sizeDiff;
                    newNodeEnd->next = 0;
                    newNode->next = newNodeEnd;
                }
                break;
            }
        }
    }

    return true;
}

void freelistClear(freelist* list) {
    if (!list || !list->memory) {
        return;
    }

    internalState* state = list->memory;
    // Invalidate the offset and size for all but the first node. The invalid
    // value will be checked for when seeking a new node from the list.
    for (u64 i = 1; i < state->maxEntries; ++i) {
        state->nodes[i].offset = INVALID_ID;
        state->nodes[i].size = INVALID_ID;
    }

    // Reset the head to occupy the entire thing.
    state->head->offset = 0;
    state->head->size = state->totalSize;
    state->head->next = 0;
}

u64 freelistFreeSpace(freelist* list) {
    if (!list || !list->memory) {
        return 0;
    }

    u64 total = 0;
    internalState* state = list->memory;
    freelistNode* node = state->head;
    while (node) {
        total += node->size;
        node = node->next;
    }

    return total;
}

freelistNode* getNode(freelist* list) {
    internalState* state = list->memory;
    for (u64 i = 1; i < state->maxEntries; ++i) {
        if (state->nodes[i].offset == INVALID_ID) {
            return &state->nodes[i];
        }
    }

    // Return nothing if no nodes are available.
    return 0;
}

void invalidateNode(freelist* list, freelistNode* node) {
    node->offset = INVALID_ID;
    node->size = INVALID_ID;
    node->next = 0;
}
