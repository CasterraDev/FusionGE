#pragma once

#include "core/application.h"

/**
 * Represents the basic game state in a game.
 * Called for creation by the application.
 */
typedef struct game {
    // The application configuration.
    appConfig appConfig;

    // Function pointer to game's initialize function.
    b8 (*initialize)(struct game* gameInstance);

    // Function pointer to game's update function.
    b8 (*update)(struct game* gameInstance, f32 deltaTime);

    // Function pointer to game's render function.
    b8 (*render)(struct game* gameInstance, f32 deltaTime);

    // Function pointer to handle resizes, if applicable.
    void (*onResize)(struct game* gameInstance, u32 width, u32 height);

    // The required state size for the game state
    u64 stateMemReq;

    // Game-specific game state. Created and managed by the game.
    void* state;

    // Application state
    void* appState;
} game;
