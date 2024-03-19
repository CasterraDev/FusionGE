#pragma once

#include "defines.h"

typedef struct clock {
    f64 startTime;
    f64 elapsed;
} clock;

// Updates the clock. Should be called just before checking elapsed time.
// Does nothing to clocks that haven't been started
FSNAPI void clockUpdate(clock* clock);

// Starts the clock. Resets elapsed time.
FSNAPI void clockStart(clock* clock);

// Stops the clock. Does not reset elapsed time.
FSNAPI void clockStop(clock* clock);