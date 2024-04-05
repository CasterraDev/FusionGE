#include "testManager.h"

#include <core/clock.h>
#include <core/logger.h>
#include <defines.h>
#include <helpers/dinoArray.h>

typedef struct test{
    PFN_test func;
    char* logOutput;
} test;

static test* tests;

void testMgrInit(){
    tests = dinoCreate(test);
}

void testMgrRegisterTest(u8 (*PFN_test)(), char* logOutput){
    test e;
    e.func = PFN_test;
    e.logOutput = logOutput;

    dinoPush(tests,e);
}

void testMgrRunTests(){
    u32 success = 0;
    u32 failed = 0;
    u32 skipped = 0;
    clock totalCnt;
    //clockStart(&totalCnt);
    u32 len = dinoLength(tests);
    for (u32 i = 0; i < len; ++i){
        clock testCnt;
        //clockStart(&testCnt);
        u8 res = tests[i].func();
        //clockUpdate(&testCnt);
        if (res == 1){
            success++;
            FINFO("Test successful. %s", tests[i].logOutput);
        }else if (res == 2){
            skipped++;
            FINFO("[SKIPPED] Test skipped. %s", tests[i].logOutput);
        }else if (res == 0){
            failed++;
            FERROR("[FAILED] Test failed. %s", tests[i].logOutput);
        }

        //clockUpdate(&totalCnt);
        FINFO("%i/%i | Successful: %i | Skipped: %i | Failed: %i | %f/%f elapsed", i + 1, len, success, skipped, failed, testCnt.elapsed, totalCnt.elapsed);
    }
    //clockUpdate(&totalCnt);
    FINFO("Successful: %i | Skipped: %i | Failed: %i | %.6f elapsed", success, skipped, failed, totalCnt.elapsed);
}