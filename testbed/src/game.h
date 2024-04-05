#pragma once

#include <defines.h>
#include <gameTypes.h>

typedef struct gameState {
    f32 deltaTime;
} gameState;

b8 gameInitialize(game* gameInst);

b8 gameUpdate(game* gameInst, f32 deltaTime);

b8 gameRender(game* gameInst, f32 deltaTime);

void gameOnResize(game* gameInst, u32 width, u32 height);