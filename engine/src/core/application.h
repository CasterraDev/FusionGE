#pragma once

#include "defines.h"

struct game;

// Application configuration.
typedef struct appConfig {
    // Window starting position x axis, if applicable.
    i16 startPosX;

    // Window starting position y axis, if applicable.
    i16 startPosY;

    // Window starting width, if applicable.
    i16 startWidth;

    // Window starting height, if applicable.
    i16 startHeight;

    // The application name used in windowing, if applicable.
    char* name;
} appConfig;


FSNAPI b8 appCreate(struct game* game_inst);

FSNAPI b8 appRun();

void appGetFramebufferSize(u32* width, u32* height);