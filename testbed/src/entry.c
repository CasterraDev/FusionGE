#include "game.h"

#include <entry.h>

// Define the function to create a game
b8 createGame(game* outGame) {
    // Application configuration.
    outGame->appConfig.startPosX = 100;
    outGame->appConfig.startPosY = 100;
    outGame->appConfig.startWidth = 1280;
    outGame->appConfig.startHeight = 720;
    outGame->appConfig.name = "Fusion Engine Testbed";
    // Assign the functions that users will be able to use.
    outGame->update = gameUpdate;
    outGame->render = gameRender;
    outGame->initialize = gameInitialize;
    outGame->onResize = gameOnResize;

    // Create the game state.
    outGame->stateMemReq = sizeof(gameState);
    // Zero out the states
    outGame->state = 0;
    outGame->appState = 0;

    return true;
}
