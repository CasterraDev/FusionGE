#include "game.h"

#include <entry.h>

#include <core/fmemory.h>

// Define the function to create a game
b8 createGame(game* outGame) {
    // Application configuration.
    outGame->appConfig.startPosX = 100;
    outGame->appConfig.startPosY = 100;
    outGame->appConfig.startWidth = 1280;
    outGame->appConfig.startHeight = 720;
    outGame->appConfig.name = "Fusion Engine Testbed";
    outGame->update = gameUpdate;
    outGame->render = gameRender;
    outGame->initialize = gameInitialize;
    outGame->onResize = gameOnResize;

    // Create the game state.
    outGame->state = fallocate(sizeof(gameState), MEMORY_TAG_GAME);

    return true;
}