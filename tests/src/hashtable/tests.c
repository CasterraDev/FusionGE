#include <helpers/hashtable.h>
#include "../testManager.h"
#include "../shouldBe.h"
#include "stdlib.h"

u8 htCreate(){
    hashtable* h = malloc(sizeof(hashtable));
    u32* x = malloc(sizeof(u32) * 3);
    hashtableCreate(sizeof(u32), 3, x, false, h);
    should_be(3, h->elementLength);
    should_be(sizeof(u32), h->elementStride);
    should_not_be(0, h->memory);
    return true;
}

u8 htSetGetInt(){
    hashtable h;
    u32 x[3];
    hashtableCreate(sizeof(u32), 3, x, false, &h);
    should_be(3, h.elementLength);
    should_be(sizeof(u32), h.elementStride);
    should_not_be(0, h.memory);
    u32 t = 5;
    b8 res = hashtableSet(&h, "test", &t);
    should_be_true(res);
    u32 y;
    b8 res2 = hashtableGet(&h, "test", &y);
    should_be_true(res2);
    should_be(t, y);
    return true;
}

typedef struct structTest {
    u64 size;
    u32 length;
} structTest;

u8 htSetGetStruct(){
    hashtable h;
    structTest x[3];
    hashtableCreate(sizeof(structTest), 3, x, false, &h);
    should_be(3, h.elementLength);
    should_be(sizeof(structTest), h.elementStride);
    should_not_be(0, h.memory);
    structTest t;
    t.size = 3;
    t.length = 7;
    b8 res = hashtableSet(&h, "test", &t);
    should_be_true(res);
    structTest y;
    b8 res2 = hashtableGet(&h, "test", &y);
    should_be_true(res2);
    should_be(t.size, y.size);
    should_be(t.length, y.length);
    return true;
}

u8 htSetGetSetGetInt(){
    hashtable h;
    u32 x[3];
    hashtableCreate(sizeof(u32), 3, x, false, &h);
    should_be(3, h.elementLength);
    should_be(sizeof(u32), h.elementStride);
    should_not_be(0, h.memory);
    u32 t = 5;
    b8 res = hashtableSet(&h, "test", &t);
    should_be_true(res);
    u32 y;
    b8 res2 = hashtableGet(&h, "test", &y);
    should_be_true(res2);
    should_be(t, y);

    u32 s = 7;
    b8 res3 = hashtableSet(&h, "test", &s);
    should_be_true(res3);
    b8 res4 = hashtableGet(&h, "test", &y);
    should_be_true(res4);
    should_be(s, y);
    return true;
}

u8 htSetGetSetGetStruct(){
    hashtable h;
    structTest x[3];
    hashtableCreate(sizeof(structTest), 3, x, false, &h);
    should_be(3, h.elementLength);
    should_be(sizeof(structTest), h.elementStride);
    should_not_be(0, h.memory);
    structTest t;
    t.size = 3;
    t.length = 7;
    b8 res = hashtableSet(&h, "test", &t);
    should_be_true(res);
    structTest y;
    b8 res2 = hashtableGet(&h, "test", &y);
    should_be_true(res2);
    should_be(t.size, y.size);
    should_be(t.length, y.length);

    structTest tx;
    tx.size = 32;
    tx.length = 76;
    b8 res3 = hashtableSet(&h, "test", &tx);
    should_be_true(res3);
    b8 res4 = hashtableGet(&h, "test", &y);
    should_be_true(res4);
    should_be(tx.size, y.size);
    should_be(tx.length, y.length);
    return true;
}

void hashtableRegisterTests(){
    testMgrRegisterTest(htCreate, "Create Hashtable");
    testMgrRegisterTest(htSetGetInt, "Get Set With Ints");
    testMgrRegisterTest(htSetGetStruct, "Get Set With Structs");
    testMgrRegisterTest(htSetGetSetGetInt, "Get Set With Ints twice");
    testMgrRegisterTest(htSetGetSetGetStruct, "Get Set With Structs twice");
}
