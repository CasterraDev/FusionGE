#include "camera.h"
#include "math/fsnmath.h"

camera cameraCreate(){
    camera cam;
    cam.position = vec3Zero();
    cam.eulerAngles = vec3Zero();
    cam.dirty = false;
    cam.view = mat4Identity();
    return cam;
}

void cameraReset(camera* cam){
    if (cam){
        cam->position = vec3Zero();
        cam->eulerAngles = vec3Zero();
        cam->dirty = false;
        cam->view = mat4Identity();
    }
}

void cameraYaw(camera* cam, f32 amount){
    if (cam){
        cam->eulerAngles.y += amount;
        cam->dirty = true;
    }
}

void cameraPitch(camera* cam, f32 amount){
    if (cam){
        cam->eulerAngles.x += amount;
        //To stop Gimball lock.
        const f32 limit = 1.55334306f;  //89 degrees
        cam->eulerAngles.x = FCLAMP(cam->eulerAngles.x, -limit, limit);
    }
}

void cameraMoveRight(camera* cam, f32 amount){
    if (cam){
        vector3 x = vec3MulScalar(cameraRight(cam), amount);
        cam->position = vec3Add(cam->position, x);
        cam->dirty = true;
    }
}

void cameraMoveLeft(camera* cam, f32 amount){
    if (cam){
        vector3 x = vec3MulScalar(cameraLeft(cam), amount);
        cam->position = vec3Add(cam->position, x);
        cam->dirty = true;
    }
}

void cameraMoveUp(camera* cam, f32 amount){
    if (cam){
        vector3 x = vec3MulScalar(cameraUp(cam), amount);
        cam->position = vec3Add(cam->position, x);
        cam->dirty = true;
    }
}

void cameraMoveDown(camera* cam, f32 amount){
    if (cam){
        vector3 x = vec3MulScalar(cameraDown(cam), amount);
        cam->position = vec3Add(cam->position, x);
        cam->dirty = true;
    }
}

void cameraMoveForward(camera* cam, f32 amount){
    if (cam){
        vector3 x = vec3MulScalar(cameraForward(cam), amount);
        cam->position = vec3Add(cam->position, x);
        cam->dirty = true;
    }
}

void cameraMoveBackward(camera* cam, f32 amount){
    if (cam){
        vector3 x = vec3MulScalar(cameraBackward(cam), amount);
        cam->position = vec3Add(cam->position, x);
        cam->dirty = true;
    }
}

void cameraPositionSet(camera* cam, vector3 pos){
    cam->position = pos;
}

vector3 cameraPositionGet(camera* cam){
    return cam->position;
}

void cameraEulerSet(camera* cam, vector3 euler){
    cam->eulerAngles = euler;
}

vector3 cameraEulerGet(camera* cam){
    return cam->eulerAngles;
}

mat4 cameraViewGet(camera* cam){
    if (cam){
        recalculateView(cam);
        return cam->view;
    }
    return mat4Identity();
}

vector3 cameraRight(camera* cam){
    mat4 v = cameraViewGet(cam);
    return mat4Right(v);
}

vector3 cameraLeft(camera* cam){
    mat4 v = cameraViewGet(cam);
    return mat4Left(v);
}

vector3 cameraUp(camera* cam){
    mat4 v = cameraViewGet(cam);
    return mat4Up(v);
}

vector3 cameraDown(camera* cam){
    mat4 v = cameraViewGet(cam);
    return mat4Down(v);
}

vector3 cameraForward(camera* cam){
    mat4 v = cameraViewGet(cam);
    return mat4Forward(v);
}

vector3 cameraBackward(camera* cam){
    mat4 v = cameraViewGet(cam);
    return mat4Backward(v);
}

void recalculateView(camera* cam){
    if (cam->dirty){
        mat4 rot = mat4EulerXyz(cam->eulerAngles.x,cam->eulerAngles.y,cam->eulerAngles.z);
        mat4 trans = mat4Translation(cam->position);

        cam->view = mat4Mul(rot, trans);
        cam->view = mat4Inverse(cam->view);

        cam->dirty = false;
    }
}
