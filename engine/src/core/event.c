#include "core/event.h"
#include "core/fmemory.h"
#include "helpers/dinoArray.h"
#include "core/logger.h"

typedef struct registeredEvent {
    void* listener;
    PF_on_event functionCallback;
} registeredEvent;

typedef struct eventCodeEntry {
    registeredEvent* events;
} eventCodeEntry;

// This should be more than enough codes...
#define MAX_MESSAGE_CODES 16384

// State structure.
// List of registered events
typedef struct eventSystemState {
    // Lookup table for event codes.
    eventCodeEntry registered[MAX_MESSAGE_CODES];
} eventSystemState;


static b8 isInit = false;
static eventSystemState* systemPtr;

b8 eventInit(u64* memoryRequirement, void* state) {
    *memoryRequirement = sizeof(eventSystemState);
    if (state == 0){
        return true;
    }
    systemPtr = state;
    isInit = true;
    return true;
}

void eventShutdown() {
    // Free the events arrays. And objects pointed to should be destroyed on their own.
    for(u16 i = 0; i < MAX_MESSAGE_CODES; ++i){
        if(systemPtr->registered[i].events != 0) {
            dinoDestroy(systemPtr->registered[i].events);
            systemPtr->registered[i].events = 0;
        }
    }
}

b8 eventRegister(u16 code, void* listener, PF_on_event on_event) {
    if(isInit == false) {
        return false;
    }

    if(systemPtr->registered[code].events == 0) {
        systemPtr->registered[code].events = dinoCreate(registeredEvent);
    }

    u64 registered_count = dinoLength(systemPtr->registered[code].events);
    for(u64 i = 0; i < registered_count; ++i) {
        if(systemPtr->registered[code].events[i].listener == listener) {
            FWARN("Event listener already added.");
            return false;
        }
    }

    // If at this point, no duplicate was found. Proceed with registration.
    registeredEvent event;
    event.listener = listener;
    event.functionCallback = on_event;
    dinoPush(systemPtr->registered[code].events, event);

    return true;
}

b8 eventUnregister(u16 code, void* listener, PF_on_event onEvent) {
    if(isInit == false) {
        return false;
    }

    // On nothing is registered for the code, return out.
    if(systemPtr->registered[code].events == 0) {
        FWARN("Event code has no listeners registered.");
        return false;
    }
    u64 registered_count = dinoLength(systemPtr->registered[code].events);
    for(u64 i = 0; i < registered_count; ++i) {
        registeredEvent e = systemPtr->registered[code].events[i];
        if(e.listener == listener && e.functionCallback == onEvent) {
            // Found one, remove it
            registeredEvent popped_event;
            dinoPopAt(systemPtr->registered[code].events, i, &popped_event);

            return true;
        }
    }

    // Not found.
    return false;
}

b8 eventFire(u16 code, void* sender, eventContext context) {
    if(isInit == false) {
        return false;
    }

    // If nothing is registered for the code, boot out.
    if(systemPtr->registered[code].events == 0) {
        return false;
    }

    u64 registered_count = dinoLength(systemPtr->registered[code].events);
    for(u64 i = 0; i < registered_count; ++i) {
        registeredEvent e = systemPtr->registered[code].events[i];
        if(e.functionCallback(code, sender, e.listener, context)) {
            // Message has been handled, do not send to other listeners.
            return true;
        }
    }

    // Not found.
    return false;
}
