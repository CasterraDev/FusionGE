#include "clock.h"

#include "platform/platform.h"

void clockUpdate(clock* clock) {
    if (clock->startTime != 0) {
        clock->elapsed = platformGetAbsoluteTime() - clock->startTime;
    }
}

void clockStart(clock* clock) {
    clock->startTime = platformGetAbsoluteTime();
    clock->elapsed = 0;
}

void clockStop(clock* clock) {
    clock->startTime = 0;
}