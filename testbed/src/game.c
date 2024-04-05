#include "game.h"

#include <core/logger.h>
#include <core/input.h>
#include <core/event.h>

b8 gameInitialize(game* gameInst) {
    FDEBUG("gameInitialize() called!");
    return true;
}

b8 gameUpdate(game* gameInst, f32 deltaTime) {

    if (inputIsKeyPressed('T')){
        eventContext context = {};
        eventFire(EVENT_CODE_DEBUG0, 0, context);
    }

    return true;
}

b8 gameRender(game* gameInst, f32 deltaTime) {
    return true;
}

void gameOnResize(game* gameInst, u32 width, u32 height) {
}
