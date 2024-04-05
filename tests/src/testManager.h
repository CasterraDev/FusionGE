#pragma once

#include <defines.h>

#define SKIPPED 2

typedef u8 (*PFN_test)();

void testMgrInit();

void testMgrRegisterTest(PFN_test, char* logOutput);

void testMgrRunTests();