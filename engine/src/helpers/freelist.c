#include "freelist.h"

#include "core/kmemory.h"
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
void returnNode(freelist* list, freelistNode* node);

void freelistCreate(u64 totalSize, u64* memoryRequirement, void* memory, freelist* outList) {
    // Enough space to hold state, plus array for all nodes.
    u64 maxEntries = (totalSize / sizeof(void*));  //NOTE: This might have a remainder, but I don't care.
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
        internalState* state = list->memory;
        fzeroMemory(list->memory, sizeof(internalState) + sizeof(freelistNode) * state->maxEntries);
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
            returnNode(list, nodeToReturn);
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
    FWARN("freelistFindBlock, no block large enough found (requested: %lluB, available: %lluB).", size, freeSpace);
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

                // Check if this then connects the range between this and the next
                // node, and if so, combine them and return the second node..
                if (node->next && node->next->offset == node->offset + node->size) {
                    node->size += node->next->size;
                    freelistNode* next = node->next;
                    node->next = node->next->next;
                    returnNode(list, next);
                }
                return true;
            } else if (node->offset > offset) {
                // Iterated beyond the space to be freed. Need a new node.
                freelistNode* newNode = getNode(list);
                newNode->offset = offset;
                newNode->size = size;

                // If there is a previous node, the new node should be inserted between this and it.
                if (previous) {
                    previous->next = newNode;
                    newNode->next = node;
                } else {
                    // Otherwise, the new node becomes the head.
                    newNode->next = node;
                    state->head = newNode;
                }

                // Double-check next node to see if it can be joined.
                if (newNode->next && newNode->offset + newNode->size == newNode->next->offset) {
                    newNode->size += newNode->next->size;
                    freelistNode* rubbish = newNode->next;
                    newNode->next = rubbish->next;
                    returnNode(list, rubbish);
                }

                // Double-check previous node to see if the newNode can be joined to it.
                if (previous && previous->offset + previous->size == newNode->offset) {
                    previous->size += newNode->size;
                    freelistNode* rubbish = newNode;
                    previous->next = rubbish->next;
                    returnNode(list, rubbish);
                }

                return true;
            }

            previous = node;
            node = node->next;
        }
    }

    FWARN("Unable to find block to be freed. Corruption possible? Or the dev who called this function is stupid.");
    return false;
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

    u64 runningTotal = 0;
    internalState* state = list->memory;
    freelistNode* node = state->head;
    while (node) {
        runningTotal += node->size;
        node = node->next;
    }

    return runningTotal;
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

void returnNode(freelist* list, freelistNode* node) {
    node->offset = INVALID_ID;
    node->size = INVALID_ID;
    node->next = 0;
}