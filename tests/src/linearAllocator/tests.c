#include <core/linearAllocator.h>
#include "../testManager.h"
#include "../shouldBe.h"

u8 linearAllocCreateDestory(){
    linearAllocator a;
    linearAllocCreate(sizeof(u64),&a);
    should_be(sizeof(u64), a.size);
    should_not_be(0, a.memory);
    should_be(0, a.alloced);
    linearAllocDestroy(&a);
    should_be(0, a.alloced);
    should_be(0, a.memory);
    should_be(0, a.size);
    return true;
}

u8 linearAllocAllSpace() {
    linearAllocator alloc;
    linearAllocCreate(sizeof(u64), &alloc);

    // Single allocation.
    void* block = linearAllocAllocate(&alloc, sizeof(u64));

    // Validate it
    should_be(sizeof(u64), alloc.alloced);
    should_not_be(0, block);

    linearAllocDestroy(&alloc);
    should_be(0, alloc.alloced);
    should_be(0, alloc.memory);
    should_be(0, alloc.size);

    return true;
}

u8 linearAllocMultipleAllSpace() {
    linearAllocator alloc;
    linearAllocCreate(sizeof(u64) * 3, &alloc);
    should_be(sizeof(u64) * 3, alloc.size);

    void* block = linearAllocAllocate(&alloc, sizeof(u64));
    should_be(sizeof(u64), alloc.alloced);
    should_not_be(0, block);
    void* block2 = linearAllocAllocate(&alloc, sizeof(u64));
    should_be(sizeof(u64) * 2, alloc.alloced);
    should_not_be(0, block2);
    void* block3 = linearAllocAllocate(&alloc, sizeof(u64));
    should_be(sizeof(u64) * 3, alloc.alloced);
    should_not_be(0, block3);

    linearAllocDestroy(&alloc);
    should_be(0, alloc.alloced);
    should_be(0, alloc.memory);
    should_be(0, alloc.size);

    return true;
}

u8 linearAllocOverflow() {
    linearAllocator alloc;
    linearAllocCreate(sizeof(u64), &alloc);

    // Single allocation.
    void* block = linearAllocAllocate(&alloc, sizeof(u64));
    FTRACE("There should be an error message telling you that there is no remaining space in the allocator. This error msg is intentional for the test.")
    void* block2 = linearAllocAllocate(&alloc, sizeof(u64));

    linearAllocDestroy(&alloc);

    return true;
}

u8 linearAllocAllocReset() {
    linearAllocator alloc;
    linearAllocCreate(sizeof(u64), &alloc);

    // Single allocation.
    void* block = linearAllocAllocate(&alloc, sizeof(u64));

    // Validate it
    should_be(sizeof(u64), alloc.alloced);
    should_not_be(0, block);

    linearAllocReset(&alloc);
    should_be(0, alloc.alloced);
    should_not_be(0, alloc.memory);
    should_be(sizeof(u64), alloc.size);

    void* block2 = linearAllocAllocate(&alloc, 2);
    should_be(2, alloc.alloced);
    should_not_be(0, block);

    linearAllocDestroy(&alloc);

    return true;
}

void linearAllocRegisterTests(){
    testMgrRegisterTest(linearAllocCreateDestory,"Linear Allocator Create & Destroy");
    testMgrRegisterTest(linearAllocAllSpace, "Linear allocator alloc all space");
    testMgrRegisterTest(linearAllocMultipleAllSpace, "Linear allocator multiple allocations for all space");
    testMgrRegisterTest(linearAllocOverflow, "Linear allocator intentional overflow.");
    testMgrRegisterTest(linearAllocAllocReset, "Linear allocator Allocate then reset then allocate again then destroy.");
}
