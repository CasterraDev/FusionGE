#include "testManager.h"

#include "linearAllocator/tests.h"
#include "hashtable/tests.h"

#include <core/logger.h>

int main() {
    // Always initalize the test manager first.
    testMgrInit();

    // TODO: add test registrations here.
    linearAllocRegisterTests();
    hashtableRegisterTests();

    FDEBUG("Starting tests...");

    testMgrRunTests();

    return 0;
}