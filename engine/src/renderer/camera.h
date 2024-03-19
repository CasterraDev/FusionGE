#pragma once

#include "defines.h"
#include "math/matrixMath.h"

typedef struct camera{
    vector3 position;
    vector3 eulerAngles;
    b8 dirty;
    /* @brief Don't get directly. Use cameraViewGet so the view can be recalculated if needed*/
    mat4 view;
} camera;

camera cameraCreate();
void cameraReset(camera* cam);

void cameraYaw(camera* cam, f32 amount);
void cameraPitch(camera* cam, f32 amount);
//void cameraRoll(camera* cam, f32 amount);

void cameraMoveRight(camera* cam, f32 amount);
void cameraMoveLeft(camera* cam, f32 amount);
void cameraMoveUp(camera* cam, f32 amount);
void cameraMoveDown(camera* cam, f32 amount);
void cameraMoveForward(camera* cam, f32 amount);
void cameraMoveBackward(camera* cam, f32 amount);

void cameraPositionSet(camera* cam, vector3 pos);
vector3 cameraPositionGet(camera* cam);
void cameraEulerSet(camera* cam, vector3 euler);
vector3 cameraEulerGet(camera* cam);
mat4 cameraViewGet(camera* cam);

vector3 cameraRight(camera* cam);
vector3 cameraLeft(camera* cam);
vector3 cameraUp(camera* cam);
vector3 cameraDown(camera* cam);
vector3 cameraForward(camera* cam);
vector3 cameraBackward(camera* cam);

void recalculateView(camera* cam);