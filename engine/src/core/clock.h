#pragma once

/**
 * @brief Prints the Memory Tags for debugging purposes
 */
#include "defines.h"

typedef struct clock {
    f64 startTime;
    f64 elapsed;
} clock;

/**
 * @brief Updates the clock. Should be called just before checking elapsed time.
 * Does nothing to clocks that haven't been started
 */
FSNAPI void clockUpdate(clock* clock);

/**
 * @brief Starts the clock. Resets elapsed time.
 */
FSNAPI void clockStart(clock* clock);

/**
 * @brief Stops the clock. Does not reset elapsed time.
 */
FSNAPI void clockStop(clock* clock);
