#include "core/input.h"
#include "core/fmemory.h"
#include "core/event.h"
#include "core/logger.h"

typedef struct keyState {
    b8 isDown;
} keyState;

typedef struct keyboardState {
    keyState keys[256];
} keyboardState;

typedef struct buttonState {
    b8 isDown;
} buttonState;

typedef struct mouseState {
    i16 x;
    i16 y;
    buttonState buttons[BUTTON_MAX_BUTTONS];
} mouseState;

typedef struct inputState {
    keyboardState keyboardCur;
    keyboardState keyboardPrev;
    mouseState mouseCur;
    mouseState mousePrev;
} inputState;

static b8 isInit = false;
static inputState* systemPtr;

void inputInit(u64* memoryRequirement, void* state){
    *memoryRequirement = sizeof(inputState);
    if (state == 0){
        return;
    }
    systemPtr = state;
    isInit = true;

    FINFO("Input system inited");
}

void inputUpdate(f64 deltaTime){
    if (!isInit){
        FERROR("Input system is not inited");
    }

    fcopyMemory(&systemPtr->keyboardPrev,&systemPtr->keyboardCur,sizeof(keyboardState));
    fcopyMemory(&systemPtr->mousePrev,&systemPtr->mouseCur,sizeof(mouseState));
}

void inputShutdown(void* state) {
    // TODO: Add shutdown routines when needed.
    systemPtr = 0;
}

void inputProcessKey(keys key, b8 pressed) {
    eventContext context;
    context.data.u16[0] = key;
    systemPtr->keyboardCur.keys[key].isDown = pressed;

    if (systemPtr->keyboardCur.keys[key].isDown != systemPtr->keyboardPrev.keys[key].isDown) {
        // Fire off an event for immediate processing.
        eventFire(pressed ? EVENT_CODE_KEY_PRESSED : EVENT_CODE_KEY_RELEASED, 0, context);
    }else{
        eventFire(EVENT_CODE_KEY_DOWN, 0, context);
    }
}

void inputProcessButton(buttons button, b8 pressed) {
    eventContext context;
    context.data.u16[0] = button;
    systemPtr->mouseCur.buttons[button].isDown = pressed;
    if (systemPtr->mouseCur.buttons[button].isDown != systemPtr->mousePrev.buttons[button].isDown){
        // Fire the event.
        eventFire(pressed ? EVENT_CODE_BUTTON_PRESSED : EVENT_CODE_BUTTON_RELEASED, 0, context);
    }else{
        eventFire(EVENT_CODE_BUTTON_DOWN, 0, context);
    }
}

void inputProcessMouseMove(i16 x, i16 y) {
    // Only process if actually different
    if (systemPtr->mouseCur.x != x || systemPtr->mouseCur.y != y) {
        // Update internal systemPtr->
        systemPtr->mouseCur.x = x;
        systemPtr->mouseCur.y = y;

        // Fire the event.
        eventContext context;
        context.data.u16[0] = x;
        context.data.u16[1] = y;
        eventFire(EVENT_CODE_MOUSE_MOVED, 0, context);
    }
}

void inputProcessMouseWheel(i8 zDelta) {
    // Fire the event.
    eventContext context;
    context.data.u8[0] = zDelta;
    eventFire(EVENT_CODE_MOUSE_WHEEL, 0, context);
}

b8 inputIsKeyDown(keys key) {
    if (!isInit) {
        return false;
    }
    return systemPtr->keyboardCur.keys[key].isDown;
}

b8 inputIsKeyUp(keys key) {
    if (!isInit) {
        return true;
    }
    return !systemPtr->keyboardCur.keys[key].isDown;
}

b8 inputIsKeyReleased(keys key) {
    if (!isInit) {
        return false;
    }
    return ((systemPtr->keyboardPrev.keys[key].isDown != systemPtr->keyboardCur.keys[key].isDown) && !systemPtr->keyboardCur.keys[key].isDown);
}

b8 inputIsKeyPressed(keys key) {
    if (!isInit) {
        return false;
    }
    return ((systemPtr->keyboardPrev.keys[key].isDown != systemPtr->keyboardCur.keys[key].isDown) && systemPtr->keyboardCur.keys[key].isDown);
}

b8 inputWasKeyPressed(keys key) {
    if (!isInit) {
        return false;
    }
    return systemPtr->keyboardPrev.keys[key].isDown;
}

b8 inputWasKeyUp(keys key) {
    if (!isInit) {
        return true;
    }
    return !systemPtr->keyboardPrev.keys[key].isDown;
}

// mouse input
b8 inputIsButtonPressed(buttons button) {
    if (!isInit) {
        return false;
    }
    return ((systemPtr->mouseCur.buttons[button].isDown != systemPtr->mousePrev.buttons[button].isDown) && systemPtr->mouseCur.buttons[button].isDown);
}

b8 inputIsButtonReleased(buttons button){
    if (!isInit) {
        return false;
    }
    return ((systemPtr->mouseCur.buttons[button].isDown != systemPtr->mousePrev.buttons[button].isDown) && !systemPtr->mouseCur.buttons[button].isDown);
}


b8 inputIsButtonUp(buttons button) {
    if (!isInit) {
        return true;
    }
    return !systemPtr->mouseCur.buttons[button].isDown;
}

b8 inputIsButtonDown(buttons button){
    if (!isInit) {
        return true;
    }
    return systemPtr->mouseCur.buttons[button].isDown;
}

b8 inputWasButtonPressed(buttons button) {
    if (!isInit) {
        return false;
    }
    return systemPtr->mousePrev.buttons[button].isDown;
}

b8 inputWasButtonUp(buttons button) {
    if (!isInit) {
        return true;
    }
    return !systemPtr->mousePrev.buttons[button].isDown;
}

void inputGetMousePosition(i32* x, i32* y) {
    if (!isInit) {
        *x = 0;
        *y = 0;
        return;
    }
    *x = systemPtr->mouseCur.x;
    *y = systemPtr->mouseCur.y;
}

void inputGetPreviousMousePosition(i32* x, i32* y) {
    if (!isInit) {
        *x = 0;
        *y = 0;
        return;
    }
    *x = systemPtr->mousePrev.x;
    *y = systemPtr->mousePrev.y;
}
