#pragma once

#include "core/application.h"
#include "core/logger.h"
#include "core/fmemory.h"
#include "gameTypes.h"

// Externally-defined function to create a game.
extern b8 createGame(game* outGame);

/**
 * The main entry point of the application.
 */
int main(void) {
    // Request the game instance from the application.
    game gameInstance;
    if (!createGame(&gameInstance)) {
        FFATAL("Could not create game!");
        return -1;
    }

    // Ensure all function pointers exist.
    if (!gameInstance.render || !gameInstance.update || !gameInstance.initialize || !gameInstance.onResize) {
        FFATAL("The game's function pointers must be assigned!");
        return -2;
    }

    // Initialization.
    if (!appCreate(&gameInstance)) {
        FINFO("Application failed to create!");
        return 1;
    }

    // Begin the game loop.
    if(!appRun()) {
        FINFO("Application did not shutdown correctly.");
        return 2;
    }

    return 0;
}
